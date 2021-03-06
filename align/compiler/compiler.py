import pathlib
import pprint

from .util import _write_circuit_graph, max_connectivity
from .read_netlist import SpiceParser
from .match_graph import read_inputs, read_setup,_mapped_graph_list,add_stacked_transistor,add_parallel_transistor,reduce_graph,define_SD,check_nodes,add_parallel_caps,add_series_res
from .write_verilog_lef import WriteVerilog, WriteSpice, print_globals,print_header,generate_lef
from .common_centroid_cap_constraint import WriteCap, check_common_centroid
from .write_constraint import WriteConst, FindArray, CopyConstFile, FindSymmetry
from .read_lef import read_lef

import logging
logger = logging.getLogger(__name__)

def generate_hierarchy(netlist, subckt, output_dir, flatten_heirarchy, unit_size_mos , unit_size_cap):
    updated_ckt_list,library = compiler(netlist, subckt, flatten_heirarchy)
    return compiler_output(netlist, library, updated_ckt_list, subckt, output_dir, unit_size_mos , unit_size_cap)

def compiler(input_ckt:pathlib.Path, design_name:str, flat=0,Debug=True):
    """
    Reads input spice file, converts to a graph format and create hierarchies in the graph    

    Parameters
    ----------
    input_ckt : input circuit path
        DESCRIPTION.
    design_name : name of top level subckt in design
        DESCRIPTION.
    flat : TYPE, flat/hierarchical
        DESCRIPTION. The default is 0.
    Debug : TYPE, writes output graph for debug
        DESCRIPTION. The default is False.

    Returns
    -------
    updated_ckt_list : list of reduced graphs for each subckt
        DESCRIPTION. reduced graphs are subckts after identification of hierarchies
    library : TYPE, list of library graphs
        DESCRIPTION.libraries are used to create hierarchies

    """
    logger.info("Starting topology identification...")
    input_dir=input_ckt.parents[0]
    logger.debug(f"Reading subckt {input_ckt}")
    sp = SpiceParser(input_ckt, design_name, flat)
    circuit = sp.sp_parser()[0]

    design_setup=read_setup(input_dir / f'{input_ckt.stem}.setup')
    logger.debug(f"template parent path: {pathlib.Path(__file__).parent}")
    lib_path=pathlib.Path(__file__).resolve().parent.parent / 'config' / 'basic_template.sp'
    logger.debug(f"template library path: {lib_path}")
    basic_lib = SpiceParser(lib_path)
    library = basic_lib.sp_parser()
    lib_path=pathlib.Path(__file__).resolve().parent.parent / 'config' / 'user_template.sp'
    user_lib = SpiceParser(lib_path)
    library += user_lib.sp_parser()
    library=sorted(library, key=lambda k: max_connectivity(k["graph"]), reverse=True)
    logger.info(f"dont use cells: {design_setup['DONT_USE_CELLS']}")
    logger.info(f"all library elements: {[ele['name'] for ele in library]}")
    if len(design_setup['DONT_USE_CELLS'])>0:
        library=[lib_ele for lib_ele in library if lib_ele['name'] not in design_setup['DONT_USE_CELLS']]

    if Debug==True:
        _write_circuit_graph(circuit["name"], circuit["graph"],
                                     "./circuit_graphs/")
        for lib_circuit in library:
            _write_circuit_graph(lib_circuit["name"], lib_circuit["graph"],
                                         "./circuit_graphs/")
    hier_graph_dict=read_inputs(circuit["name"],circuit["graph"])

    updated_ckt_list = []
    check_duplicates={}
    for circuit_name, circuit in hier_graph_dict.items():
        logger.debug(f"START MATCHING in circuit: {circuit_name}")
        G1 = circuit["graph"]
        if circuit_name in design_setup['DIGITAL']:
            mapped_graph_list = _mapped_graph_list(G1, library, design_setup['POWER']+design_setup['GND'] ,design_setup['CLOCK'], True )
        else:
            define_SD(G1,design_setup['POWER'],design_setup['GND'], design_setup['CLOCK'])
            logger.debug(f"no of nodes: {len(G1)}")
            add_parallel_caps(G1)
            add_series_res(G1)
            add_stacked_transistor(G1)
            add_parallel_transistor(G1)
            initial_size=len(G1)
            delta =1
            while delta > 0:
                logger.debug("CHECKING stacked transistors")
                add_stacked_transistor(G1)
                delta = initial_size - len(G1)
                initial_size = len(G1)
            mapped_graph_list = _mapped_graph_list(G1, library, design_setup['POWER']+design_setup['GND']  ,design_setup['CLOCK'], False )
        # reduce graph converts input hierarhical graph to dictionary
        updated_circuit, Grest = reduce_graph(G1, mapped_graph_list,library,check_duplicates,design_setup)
        check_nodes(updated_circuit)
        updated_ckt_list.extend(updated_circuit)


        stop_points=design_setup['POWER']+design_setup['GND']+design_setup['CLOCK']
        if circuit_name not in design_setup['DIGITAL']:
            symmetry_blocks=FindSymmetry(Grest, circuit["ports"], circuit["ports_weight"], stop_points)
            for symm_blocks in symmetry_blocks.values():
                if isinstance(symm_blocks, dict) and "graph" in symm_blocks.keys():
                    logger.debug(f"added new hierarchy: {symm_blocks['name']} {symm_blocks['graph'].nodes()}")
                    updated_ckt_list.append(symm_blocks)

        updated_ckt_list.append({
            "name": circuit_name,
            "graph": Grest,
            "ports": circuit["ports"],
            "ports_weight": circuit["ports_weight"],
            "ports_match": circuit["connection"],
            "size": len(Grest.nodes())
        })

        lib_names=[lib_ele['name'] for lib_ele in library]
        for lib_name, dupl in check_duplicates.items():
            if len(dupl)>1:
                print(dupl)
                lib_names+=[lib_name+'_type'+str(n) for n in range(len(dupl))]
    return updated_ckt_list, lib_names

def compiler_output(input_ckt, lib_names , updated_ckt_list, design_name:str, result_dir:pathlib.Path, unit_size_mos=12, unit_size_cap=12):
    """
    search for constraints and write output in verilog format
    Parameters
    ----------
    input_ckt : TYPE. input circuit path
        DESCRIPTION.Used to take designer provided constraints
    library : TYPE. list of library graphs used
        DESCRIPTION.
    updated_ckt_list : TYPE. list of reduced circuit graph
        DESCRIPTION. this list is used to generate constraints
    design_name : TYPE. name of top level design
        DESCRIPTION.
    result_dir : TYPE. directoy path for writing results
        DESCRIPTION. writes out a verilog netlist, spice file and constraints
    unit_size_mos : TYPE, Used as parameter for cell generator
        DESCRIPTION. Cells are generated on a uniform grid
    unit_size_cap : TYPE, Used as parameter for cell generator
        DESCRIPTION. The default is 12.

    Raises
    ------
    SystemExit: We don't hanadle floating ports in design. They should be removed before hand
        DESCRIPTION.

    Returns
    -------
    primitives : Input parmeters for cell generator
        DESCRIPTION.

    """
    
    if not result_dir.exists():
        result_dir.mkdir()
    logger.debug(f"Writing results in dir: {result_dir} {updated_ckt_list}")
    input_dir=input_ckt.parents[0]
    VERILOG_FP = open(result_dir / f'{design_name}.v', 'w')
    printed_mos = []
    logger.debug("writing spice file for cell generator")

    ## File pointer for spice generator
    SP_FP = open(result_dir / (design_name + '_blocks.sp'), 'w')
    print_header(VERILOG_FP, design_name)
    design_setup=read_setup(input_dir / (input_ckt.stem + '.setup'))
    try:
        POWER_PINS = [design_setup['GND'][0],design_setup['POWER'][0]]
    except (IndexError, ValueError):
        POWER_PINS=[]
        logger.error("no power and gnd defination, correct setup file")

    #read lef to not write those modules as macros
    lef_path = pathlib.Path(__file__).resolve().parent.parent / 'config'
    ALL_LEF = read_lef(lef_path)
    logger.debug(f"Available library cells: {', '.join(ALL_LEF)}")
    # local hack for deisgn vco_dtype,
    #there requirement is different size for nmos and pmos
    if 'vco_dtype_12' in  design_name:
        unit_size_mos=37
    generated_module=[]
    primitives = {}
    duplicate_modules =[]
    for member in updated_ckt_list:
        name = member["name"]
        if name in duplicate_modules:
            continue
        else:
            duplicate_modules.append(name)
        logger.debug(f"Found module: {name}")
        inoutpin = []
        logger.debug(f'found ports match: {member["ports_match"]}')
        floating_ports=[]
        if member["ports_match"]:
            for key in member["ports_match"].keys():
                if key not in POWER_PINS:
                    inoutpin.append(key)
            if member["ports"]:
                logger.debug(f'Found module ports: {member["ports"]}')
                floating_ports = set(inoutpin) - set(member["ports"]) - set(design_setup['POWER']) -set(design_setup['GND'])
                if 'mos_body' in member:
                    floating_ports = floating_ports - set(member["mos_body"])

                if len(list(floating_ports))> 0:
                    logger.error(f"floating ports found: {name} {floating_ports}")
                    raise SystemExit('Please remove floating ports')
        else:
            inoutpin = member["ports"]

        graph = member["graph"].copy()
        logger.debug(f"Reading nodes from graph: {graph}")
        for node, attr in graph.nodes(data=True):
            #lef_name = '_'.join(attr['inst_type'].split('_')[0:-1])
            if 'net' in attr['inst_type']: continue
            #Dropping floating ports
            #if attr['ports'
            lef_name = attr['inst_type'].split('_type')[0]
            if "values" in attr and (lef_name in ALL_LEF):
                block_name, block_args = generate_lef(
                    lef_name, attr["values"],
                    primitives, unit_size_mos, unit_size_cap)
                block_name_ext = block_name.replace(lef_name,'')
                logger.debug(f"Created new lef for: {block_name}")
                # Only unit caps are generated
                if  block_name.lower().startswith('cap'):
                    graph.nodes[node]['inst_type'] = block_args['primitive']
                    block_args['primitive']=block_name
                else:
                    graph.nodes[node]['inst_type'] = block_name

                if block_name in primitives:
                    assert block_args == primitives[block_name]
                else:
                    primitives[block_name] = block_args
            else:
                logger.info(f"No physical information found for: {name}")


        if name in ALL_LEF:
            logger.debug(f"writing spice for block: {name}")
            ws = WriteSpice(graph, name+block_name_ext, inoutpin, updated_ckt_list, lib_names)
            ws.print_subckt(SP_FP)
            ws.print_mos_subckt(SP_FP,printed_mos)
            continue

        logger.debug(f"generated data for {name} : {pprint.pformat(primitives, indent=4)}")
        if name not in  ALL_LEF or name.split('_type')[0] not in ALL_LEF:
            ws = WriteSpice(graph, name, inoutpin, updated_ckt_list, lib_names)
            ws.print_subckt(SP_FP)
            ws.print_mos_subckt(SP_FP,printed_mos)

            logger.debug(f"call verilog writer for block: {name}")
            wv = WriteVerilog(graph, name, inoutpin, updated_ckt_list, POWER_PINS)
            logger.debug(f"call array finder for block: {name}")
            all_array=FindArray(graph, input_dir, name,member["ports_weight"] )
            logger.debug(f"Copy const file for: {name}")
            const_file = CopyConstFile(name, input_dir, result_dir)
            logger.debug(f"cap constraint gen for block: {name}")

            ##Removinf constraints to fix cascoded cmc
            if name not in design_setup['DIGITAL'] and name not in lib_names:
                logger.debug(f"call constraint generator writer for block: {name}")
                stop_points=design_setup['POWER']+design_setup['GND']+design_setup['CLOCK']
                WriteConst(graph, result_dir, name, inoutpin, member["ports_weight"],all_array, stop_points)
                WriteCap(graph, result_dir, name, unit_size_cap,all_array)
                check_common_centroid(graph,const_file,inoutpin)

            wv.print_module(VERILOG_FP)
            generated_module.append(name)
    if len(POWER_PINS)>0:
        print_globals(VERILOG_FP,POWER_PINS)
    SP_FP.close()

    logger.info("Topology identification done !!!")
    logger.info(f"OUTPUT verilog netlist at: {result_dir}/{design_name}.v")
    logger.info(f"OUTPUT spice netlist at: {result_dir}/{design_name}_blocks.sp")
    logger.info(f"OUTPUT const file at: {result_dir}/{design_name}.const")
    print("compilation stage done")
    return primitives
