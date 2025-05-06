
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

    XBT_INFO("Begin elasticity... VMs available:");
    auto vms = simgrid::s4u::Engine::get_instance()->get_filtered_hosts([](const simgrid::s4u::Host* host) 
    { return (dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)); });
    for (auto vm : vms)
    {
      XBT_INFO("VM: %s state %s", vm->get_cname(), simgrid::s4u::VirtualMachine::to_c_str(dynamic_cast<simgrid::s4u::VirtualMachine const*>(vm)->get_state()));
    }    
    
    auto tasks = getTasks();
    for (auto etm : tasks){ 
      std::vector<double> lv = etm->getCPULoads();
      double avgLoad = std::accumulate(lv.begin(), lv.end(), 0.0) / lv.size() * 100;
      std::string s = "";
      for (auto v : lv) s += std::to_string(v)+" " ;
  
      int execInSlot = etm->getCounterExecSlot();
      XBT_INFO("(before) %s %f %d %ld %ld %d stats", etm->getServiceName().c_str(), avgLoad, etm->getInstanceAmount(), etm->getAmountOfWaitingRequests(),
        etm->getAmountOfExecutingRequests(), execInSlot);
  
      etm->resetCounterExecSlot();
  
      if (avgLoad > upperCPUThresh_) {
        auto next_host = getNextHost(1);
        if (next_host == nullptr)
        {
          XBT_INFO("no more hosts to add to service");
        }
        else
        {
          etm->addHost(next_host);
        }
      } else if (avgLoad < lowCPUThresh_ && etm->getInstanceAmount() > 1) {
        // if more than one instance, remove one
        XBT_INFO("remove host");
        etm->removeHost(0);
      }
  
      etm->resetCounterExecSlot();
    }
    XBT_INFO("Finished elasticity... VMs available:");
    vms = simgrid::s4u::Engine::get_instance()->get_filtered_hosts([](const simgrid::s4u::Host* host) 
    { return (dynamic_cast<simgrid::s4u::VirtualMachine const*>(host)); });
    for (auto vm : vms)
    {
      XBT_INFO("VM: %s state %s", vm->get_cname(), simgrid::s4u::VirtualMachine::to_c_str(dynamic_cast<simgrid::s4u::VirtualMachine const*>(vm)->get_state()));
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
      XBT_INFO("VM %s cores %f", vm->get_cname(), vm->get_core_count() * weight_cores);
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
        XBT_INFO("Host %s available %f", h->get_cname(), (h->get_core_count() - cores_usage[h->get_name()]));
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
