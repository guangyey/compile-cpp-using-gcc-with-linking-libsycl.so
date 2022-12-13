#include <sycl/sycl.hpp>
#include <iostream>

struct DPCPPDevicePool {
  std::vector<std::unique_ptr<sycl::device>> devices;
  std::vector<std::unique_ptr<sycl::context>> contexts;
  std::mutex devices_mutex;
} gDevPool;

static std::vector<std::unique_ptr<sycl::queue>> default_queues;

static void enumDevices(
    std::vector<std::unique_ptr<sycl::device>>& devices) {
  std::vector<sycl::device> root_devices;
  auto platform_list = sycl::platform::get_platforms();
  // Enumerated root devices(GPU cards) from GPU Platform firstly.
  for (const auto& platform : platform_list) {
    if (platform.get_backend() != sycl::backend::ext_oneapi_level_zero)
      continue;
    auto device_list = platform.get_devices();
    for (const auto& device : device_list) {
      if (device.is_gpu()) {
        root_devices.push_back(device);
      }
    }
  }

  if (true) {
    constexpr sycl::info::partition_property partition_by_affinity =
        sycl::info::partition_property::partition_by_affinity_domain;
    constexpr sycl::info::partition_affinity_domain next_partitionable =
        sycl::info::partition_affinity_domain::next_partitionable;
    for (const auto& root_device : root_devices) {
      std::vector<int> deviceids_eachcard = {};
      try {
        auto sub_devices =
            root_device.create_sub_devices<partition_by_affinity>(
                next_partitionable);
        for (auto& s_dev : sub_devices) {
          devices.push_back(std::make_unique<sycl::device>(s_dev));
          deviceids_eachcard.push_back(devices.size() - 1);
        }
      } catch (sycl::exception& e) {
        if (e.code() != sycl::errc::feature_not_supported &&
            e.code() != sycl::errc::invalid) {
          throw std::runtime_error(
              std::string("Failed to apply tile partition: ") + e.what());
        }
        devices.push_back(std::make_unique<sycl::device>(root_device));
        deviceids_eachcard.push_back(devices.size() - 1);
      }
    }
  } else {
    for (const auto& root_device : root_devices) {
      std::vector<int> deviceids_eachcard = {};
      devices.push_back(std::make_unique<sycl::device>(root_device));
      deviceids_eachcard.push_back(devices.size() - 1);
    }
  }
}

static void initGlobalDevicePoolState() {
  enumDevices(gDevPool.devices);
  gDevPool.contexts.resize(1);
  gDevPool.contexts[0] = std::make_unique<sycl::context>(
      gDevPool.devices[0]->get_platform().ext_oneapi_get_default_context());
}

bool dpcppGetDeviceCount(int* deviceCount) {
  initGlobalDevicePoolState();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  *deviceCount = (int)gDevPool.devices.size();
  return true;
}

sycl::context dpcppGetDeviceContext(int device) {
  return *gDevPool.contexts[0];
}

sycl::device dpcppGetRawDevice(int device_id) {
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  return *gDevPool.devices[device_id];
}

static void initQueuePool() {
  int num_gpus;
  dpcppGetDeviceCount(&num_gpus);
  if (default_queues.size() == 0) {
    default_queues.resize(num_gpus);
  }

  // init default queues
  for (int i = 0; i < num_gpus; ++i) {
    auto queue = sycl::queue(
            dpcppGetDeviceContext(i),
            dpcppGetRawDevice(i),
            {sycl::property::queue::in_order(),
             sycl::property::queue::enable_profiling()});

    default_queues[i] = std::make_unique<sycl::queue>(queue);
  }
}

sycl::queue dpcppGetCurrentQueue(int device_id = 0) {
  return *default_queues[device_id];
}

int DATA_SIZE = 1024;

int main() {
    int num_gpus;
    if (!dpcppGetDeviceCount(&num_gpus)) {
        std::cout << "device count error" << std::endl;
    }
    std::cout << "device_count = " << num_gpus << std::endl;
    initQueuePool();

    auto device_data = sycl::malloc_device<int>(DATA_SIZE, dpcppGetCurrentQueue());
    int* host_data = (int *)malloc(DATA_SIZE * sizeof(DATA_SIZE));

    for (int i = 0; i < DATA_SIZE; i++) {
      host_data[i] = 2;
    }

    dpcppGetCurrentQueue().memcpy(device_data, host_data, DATA_SIZE * sizeof(int));
    dpcppGetCurrentQueue().wait();
  
    // dpcppGetCurrentQueue().parallel_for(DATA_SIZE, [=](sycl::id<1> idx) {
    //     device_data[idx] = idx;
    // });
    // dpcppGetCurrentQueue().wait();
    // dpcppGetCurrentQueue().memcpy(host_data, device_data, DATA_SIZE * sizeof(int));
    // dpcppGetCurrentQueue().wait();
    // for (int i = 1020; i < DATA_SIZE; i++) {
    //     std::cout << host_data[i] << std::endl;
    // }
    std::cout << "finish!" << std::endl;
}
