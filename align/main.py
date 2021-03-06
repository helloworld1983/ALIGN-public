import pathlib

from .compiler import generate_hierarchy
from .primitive import generate_primitive
from .pnr import generate_pnr
from .gdsconv.json2gds import convert_GDSjson_GDS
from .utils.gds2png import generate_png

import logging
logger = logging.getLogger(__name__)

def schematic2layout(netlist_dir, pdk_dir, netlist_file=None, subckt=None, working_dir=None, flatten=False, unit_size_mos=10, unit_size_cap=10, nvariants=1, effort=0, check=False, extract=False, log_level=None, generate=False):

    if log_level:
        logging.getLogger().setLevel(logging.getLevelName(log_level))

    if working_dir is None:
        working_dir = pathlib.Path.cwd().resolve()
    if not working_dir.is_dir():
        logger.error(f"Working directory {working_dir} doesn't exist. Please enter a valid directory path.")
        raise FileNotFoundError(2, 'No such working directory', working_dir)

    pdk_dir = pathlib.Path(pdk_dir).resolve()
    if not pdk_dir.is_dir():
        logger.error(f"PDK directory {pdk_dir} doesn't exist. Please enter a valid directory path")
        raise FileNotFoundError(2, 'No such pdk directory', pdk_dir)

    netlist_dir = pathlib.Path(netlist_dir).resolve()
    if not netlist_dir.is_dir():
        logger.error(f"Netlist directory {netlist_dir} doesn't exist. Please enter a valid directory path")
        raise FileNotFoundError(2, 'No such netlist directory', netlist_dir)

    netlist_files = [x for x in netlist_dir.iterdir() if x.is_file() and x.suffix in ('.sp', '.cdl')]
    if not netlist_files:
        logger.error(
            f"No spice files found in netlist directory {netlist_dir}. Exiting...")
        raise FileNotFoundError(2, 'No matching netlist files', f'{netlist_dir}/*(.sp|.cdl)')

    if netlist_file:
        netlist_file = pathlib.Path(netlist_file).resolve()
        netlist_files = [x for x in netlist_files if netlist_file == x]
        if not netlist_files:
            logger.error(
                f"No spice files {netlist_file} found in netlist directory. Exiting...")
            raise FileNotFoundError(2, 'No such netlist file', netlist_file)

    if subckt is None:
        assert len(netlist_files) == 1, "Encountered multiple spice files. Cannot infer top-level circuit"
        subckt = netlist_files[0].stem

    # Create directories for each stage
    topology_dir = working_dir / '1_topology'
    topology_dir.mkdir(exist_ok=True)
    primitive_dir = (working_dir / '2_primitives')
    primitive_dir.mkdir(exist_ok=True)
    pnr_dir = working_dir / '3_pnr'
    pnr_dir.mkdir(exist_ok=True)

    results = []
    for netlist in netlist_files:
        logger.info(f"READ file: {netlist} subckt={subckt}, flat={flatten}")
        # Generate hierarchy
        primitives = generate_hierarchy(netlist, subckt, topology_dir, flatten, unit_size_mos , unit_size_cap)
        # Generate primitives
        for block_name, block_args in primitives.items():
            logger.debug(f"Generating primitive: {block_name}")
            generate_primitive(block_name, **block_args, pdkdir=pdk_dir, outputdir=primitive_dir)
        # Copy over necessary collateral & run PNR tool
        variants = generate_pnr(topology_dir, primitive_dir, pdk_dir, pnr_dir, subckt, nvariants, effort, check, extract)
        results.append( (netlist, variants))
        assert len(variants) >= 1, f"No layouts were generated for {netlist}. Cannot proceed further. See LOG/compiler.log for last error."
        # Generate necessary output collateral into current directory
        for variant, filemap in variants.items():
            convert_GDSjson_GDS(filemap['gdsjson'], working_dir / f'{variant}.gds')
            print("Use KLayout to visualize the generated GDS:",working_dir / f'{variant}.gds')
            (working_dir / filemap['lef'].name).write_text(filemap['lef'].read_text())
            if check:
                if filemap['errors'] > 0:
                    (working_dir / filemap['errfile'].name).write_text(filemap['errfile'].read_text())
            if extract:
                (working_dir / filemap['cir'].name).write_text(filemap['cir'].read_text())
            # Generate PNG
            if generate:
                generate_png(working_dir, variant)
    return results

