
#include <vector>
#include <iostream>
#include <numeric>
#include <simgrid/s4u/Actor.hpp>

#include "ElasticPolicy.hpp"
#include "ElasticTask.hpp"

#define weight_cores 0.5

// using namespace simgrid;
// using namespace s4u;
namespace sg_microserv {

XBT_LOG_NEW_DEFAULT_CATEGORY(ElasticPolicyCPUKubLeast, "Elastic tasks policy manager");

/*  If it is using virtual machines and it does not receive a host:
    Finds a node using kubernetes LeastAllocated strategy (default) 
        https://kubernetes.io/docs/concepts/scheduling-eviction/kube-scheduler/
        https://kubernetes.io/docs/reference/scheduling/config/
    LeastAllocated: 
    score = (capacity - requested) / capacity * 10
    Here, we consider only CPU, but memory can be easily added 
*/

ElasticPolicyCPUKubLeast::ElasticPolicyCPUKubLeast(double interval, double uCPUT, double lCPUT)
  : ElasticPolicy(interval), upperCPUThresh_(uCPUT), lowCPUThresh_(lCPUT)
{
    hostPool_ = simgrid::s4u::Engine::get_instance()->get_filtered_hosts([](const simgrid::s4u::Host* host) 
                                    { return !dynamic_cast<simgrid::s4u::VirtualMachine const*>(host); });
}


void ElasticPolicyCPUKubLeast::run() {
  int instanceToStartIndex = 0;

  XBT_INFO("ElasticPolicyCPUKubLeast activated");
  while (isActive()) {
    // wait until next scaling
    simgrid::s4u::this_actor::sleep_for(getUpdateInterval());
   
    auto tasks = getTasks();
    for (auto etm : tasks){ 
      std::vector<double> lv = etm->getCPULoads();
      double avgLoad = std::accumulate(lv.begin(), lv.end(), 0.0) / lv.size() * 100;
      std::string s = "";
      for (auto v : lv) s += std::to_string(v)+" " ;
  
      int execInSlot = etm->getCounterExecSlot();
  
      etm->resetCounterExecSlot();
  
      if (avgLoad > upperCPUThresh_) {
        auto next_host = getNextHost(1);
        if (next_host == nullptr)
        {
          XBT_INFO("Service %s needs more replicas but there is not host to receive it. Average Load: %f", etm->getServiceName().c_str(), avgLoad);
        }
        else
        {
          XBT_INFO("Service %s added a new replica in host %s. Average Load: %f", etm->getServiceName().c_str(), next_host->get_cname(), avgLoad);
          etm->addHost(next_host);
        }
      } else if (avgLoad < lowCPUThresh_ && etm->getInstanceAmount() > 1) {
        // if more than one instance, remove one
        XBT_INFO("Service %s can remove one replica. Average Load: %f", etm->getServiceName().c_str(), avgLoad);
        etm->removeHost(0);
      }
  
      etm->resetCounterExecSlot();
    }
  }
}

simgrid::s4u::Host* ElasticPolicyCPUKubLeast::getNextHost(double cores_demanded)
{
    auto vms = simgrid::s4u::Engine::get_instance()->get_filtered_hosts([](const simgrid::s4u::Host* host) 
    { return (dynamic_cast<simgrid::s4u::VirtualMachine const*>(host) && dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)->get_state() == simgrid::s4u::VirtualMachine::State::CREATED); });
    std::map<std::string, double> cores_usage;
    auto vms_ptr = vms.begin();
    // Adapt the number of cores, because they share the core
    cores_demanded *= weight_cores;
    while (vms_ptr != vms.end())
    {
      const simgrid::s4u::VirtualMachine *vm = dynamic_cast<simgrid::s4u::VirtualMachine const*>(*vms_ptr);
        if (cores_usage.find(vm->get_pm()->get_name()) == cores_usage.end())
        {
            cores_usage[vm->get_pm()->get_name()] = vm->get_core_count() * weight_cores;
        }
        else
        {
            cores_usage[vm->get_pm()->get_name()] += vm->get_core_count() * weight_cores;
        }
        vms_ptr++;
    }
    simgrid::s4u::Host* host = nullptr;
    double load = std::numeric_limits<double>::max();
    for(auto h : hostPool_)
    {
        if (cores_demanded > (h->get_core_count() - cores_usage[h->get_name()]))
            continue;
        // score = (capacity - requested) / capacity
        double load_host = ((h->get_core_count() - cores_usage[h->get_name()]) / h->get_core_count());
        if (load > load_host)
        {
            load = load_host;
            host = h;
        }
    }
    return host;
}

}  // namespace sg_microserv
