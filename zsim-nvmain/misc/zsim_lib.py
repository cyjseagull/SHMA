import h5py # presents HDF5 files as numpy arrays
import numpy as np

from parse_config import parse_config

def get_results():
  a = parse_config("./out.cfg")
  f = h5py.File('zsim-ev.h5', 'r')
  dset = f["stats"]["root"]

  def get(a, keys):
    for k in keys.split('.'):
      a = a[k]
    return a

  def total_cores(a):
    out = 0
    for core in get(a, 'sys.cores').values():
      out += int(core['cores'])
    return str(out)

  def cache_levels(a):
    for k, v in get(a, 'sys.caches').items():
      if v['parent'] == 'mem':
        assert(k.lower().startswith('l'))
        return k[1:]
    assert(False)

  def cache_shared_cores(a, cache_name):
    all_cores, = get(a, 'sys.cores').values()
    cache = get(a, 'sys.caches.' + cache_name)
    return str(int(all_cores['cores'])/int(cache['caches']))

  def core_type(a):
    all_cores, = get(a, 'sys.cores').values()
    if all_cores['type'] == 'OOO':
      return 'rob'
    else:
      return 'interval'

  def rob_timer_in_order(a):
    all_cores, = get(a, 'sys.cores').values()
    if all_cores['type'] == 'Simple':
      return '1'
    else:
      return '0'

  def cache_loads_stores(a, dset, zsim_cache_name, sniper_cache_name):
    # returns padded list
    def plist(l, n):
      return (list(l) + [0]*n)[:n]
    zcn = zsim_cache_name
    scn = sniper_cache_name
    is_l1 = zsim_cache_name in 'l1d l1i'.split()
    n_cores = len(core_instrs(a, dset))

    # We have to use plist() to have n_cores entries
    # in last level caches. Each entry is for each core.
    out = {scn + '.load-misses': plist(dset[zcn][-1]['mGETS'], n_cores),
           scn + '.loads': plist((dset[zcn][-1]['fhGETS'] if is_l1 else 0)+
                                  dset[zcn][-1]['hGETS'] +
                                  dset[zcn][-1]['mGETS'],
                                  n_cores),
           # TODO: check with bigger program
           scn + '.store-misses': plist(dset[zcn][-1]['mGETXIM'] +
                                        dset[zcn][-1]['mGETXSM'],
                                        n_cores),
           scn + '.stores': plist((dset[zcn][-1]['fhGETX'] if is_l1 else 0) +
                                   dset[zcn][-1]['hGETX'] +
                                   dset[zcn][-1]['mGETXIM'] +
                                   dset[zcn][-1]['mGETXSM'],
                                   n_cores)}
    return out

  def core_instrs(a, dset):
    core_name, = get(a, 'sys.cores').keys()
    return list(dset[core_name][-1]['instrs'])

  results = {
    'config': {'general/total_cores': total_cores(a),
              'perf_model/branch_predictor/mispredict_penalty': '17', # HARD
              'perf_model/cache/levels': cache_levels(a),

              'perf_model/core/frequency': str(float(get(a, 'sys.frequency'))/1000),
              'perf_model/core/interval_timer/dispatch_width': '4',  # HARD
              'perf_model/core/interval_timer/window_size': '128',   # HARD
              'perf_model/core/type': core_type(a),
              'perf_model/core/rob_timer/in_order': rob_timer_in_order(a),

              'perf_model/dram/controllers_interleaving': get(a, 'sys.mem.controllers'),
              'perf_model/dram/num_controllers': get(a, 'sys.mem.controllers'),

              'perf_model/l1_dcache/associativity': get(a, 'sys.caches.l1d.array.ways'),
              'perf_model/l1_dcache/cache_block_size': get(a, 'sys.lineSize'),
              'perf_model/l1_dcache/cache_size': str(int(get(a, 'sys.caches.l1d.size'))/1024),
              'perf_model/l1_dcache/data_access_time': get(a, 'sys.caches.l1d.latency'),

              'perf_model/l1_icache/associativity': get(a, 'sys.caches.l1i.array.ways'),
              'perf_model/l1_icache/cache_block_size': get(a, 'sys.lineSize'),
              'perf_model/l1_icache/cache_size': str(int(get(a, 'sys.caches.l1i.size'))/1024),
              'perf_model/l1_icache/data_access_time': get(a, 'sys.caches.l1i.latency'),

              'perf_model/l2_cache/associativity': get(a, 'sys.caches.l2.array.ways'),
              'perf_model/l2_cache/cache_block_size': get(a, 'sys.lineSize'),
              'perf_model/l2_cache/cache_size': str(int(get(a, 'sys.caches.l2.size'))/1024),
              'perf_model/l2_cache/data_access_time': get(a, 'sys.caches.l2.latency'),
              'perf_model/l2_cache/dvfs_domain': 'core',
              'perf_model/l2_cache/shared_cores': cache_shared_cores(a, 'l2'),

              'perf_model/l3_cache/associativity': get(a, 'sys.caches.l3.array.ways'),
              'perf_model/l3_cache/cache_size': str(int(get(a, 'sys.caches.l3.size'))/1024),
              'perf_model/l3_cache/dvfs_domain': 'global',
              'perf_model/l3_cache/shared_cores': cache_shared_cores(a, 'l3')},

   'results': {'dram.reads': [635, 0],
               'dram.writes': [0, 0],
               'fs_to_cycles_cores': [2.66e-06, 2.66e-06],
               'global.time': 177300000000,
               'global.time_begin': 0,
               'global.time_end': 177300000000,
               'interval_timer.uop_branch': [21372, 21555],
               'interval_timer.uop_fp_addsub': [19518, 19518],
               'interval_timer.uop_fp_muldiv': [15360, 15360],
               'interval_timer.uop_generic': [268651, 269088],
               'interval_timer.uop_load': [254884, 254896],
               'interval_timer.uop_store': [78007, 78248],
               'network.shmem-1.bus.num-packets': [0, 0],
               'network.shmem-1.bus.time-used': [0, 0],
               'performance_model.elapsed_time': [177306375244, 176602617436],
               'performance_model.instruction_count': core_instrs(a, dset)}}
  results['results'].update(cache_loads_stores(a, dset, 'l1d', 'L1-D'))
  results['results'].update(cache_loads_stores(a, dset, 'l1i', 'L1-I'))
  results['results'].update(cache_loads_stores(a, dset, 'l2', 'L2'))
  results['results'].update(cache_loads_stores(a, dset, 'l3', 'L3'))

  return results
