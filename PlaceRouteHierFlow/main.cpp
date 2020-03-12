#include <string>
#include <iostream>
#include <iomanip>
#include "./PnRDB/datatype.h"
#include "./PnRDB/PnRdatabase.h"
#include "./placer/Placer.h"
#include "./router/Router.h"
#include "./cap_placer/capplacer.h"
#include "./MNA/MNASimulation.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <sstream>
#include <thread>

using std::string;
using std::cout;
using std::endl;

double ConstGraph::LAMBDA=1000;
double ConstGraph::GAMAR=30;
double ConstGraph::BETA=100;
double ConstGraph::SIGMA=1000;
double ConstGraph::PHI=1500;

static void save_state( const PnRdatabase& DB, const PnRDB::hierNode& current_node, int lidx,
			const string& opath, const string& tag, const string& ltag, bool skip)
{
  if ( skip) return;

  string ofn;
  if ( lidx >= 0) {
    ofn = opath+current_node.name + "_" + std::to_string(lidx) + tag + ".db.json";
  } else {
    ofn = opath+current_node.name + tag + ".db.json";
  }
  DB.WriteDBJSON(current_node,ofn);
  std::cout << ltag << std::endl;
}

static void route_single_variant( PnRdatabase& DB, const PnRDB::Drc_info& drcInfo, PnRDB::hierNode& current_node, int lidx, const string& opath, const string& binary_directory, bool skip_saving_state, bool adr_mode)
{
  std::cout<<"Checkpoint: work on layout "<<lidx<<std::endl;
  DB.Extract_RemovePowerPins(current_node);

  std::cout<<"Checkpoint : before route"<<std::endl;
  DB.PrintHierNode(current_node);

  //DB.WriteJSON (current_node, true, false, false, false, current_node.name+"_PL_"+std::to_string(lidx), drcInfo, opath); //block net powernet powergrid

  Router curr_route;

  bool NEW_GLOBAL_ROUTER = 1;
  double rate = 0.1;

  if ( NEW_GLOBAL_ROUTER) {
    // Gcell Global Routing
    save_state( DB, current_node, lidx, opath, ".pre_gr", "Starting Gcell Global Routing", skip_saving_state);
    int global_router_mode = 4;
    if ( adr_mode) {
      global_router_mode = 6;
    }
    curr_route.RouteWork(global_router_mode, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 0, 6, binary_directory, rate);
    save_state( DB, current_node, lidx, opath, ".post_gr", "Ending Gcell Global Routing", skip_saving_state);

    std::cout << "***WriteGcellGlobalRoute Debugging***" << std::endl;
    DB.WriteGcellGlobalRoute(current_node, current_node.name+"_GcellGlobalRoute_"+std::to_string(lidx)+".json", opath);
    std::cout << "***End WriteGcellGlobalRoute Debugging***" << std::endl;

    save_state( DB, current_node, lidx, opath, ".pre_dr", "Starting Gcell Detail Routing", skip_saving_state);
    curr_route.RouteWork(5, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 0, 6, binary_directory, rate);
    save_state( DB, current_node, lidx, opath, ".post_dr", "Ending Gcell Detail Routing", skip_saving_state);
  } else {
    // Global Routing (old version)
    save_state( DB, current_node, lidx, opath, ".pre_gr", "Checkpoint : global route", skip_saving_state);
    curr_route.RouteWork(0, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 0, 6, binary_directory, rate);
    save_state( DB, current_node, lidx, opath, ".post_gr", "Checkpoint : after global route", skip_saving_state);

    DB.WriteJSON (current_node, true, true, false, false, current_node.name+"_GR_"+std::to_string(lidx), drcInfo, opath);
    // The following line is used to write global route results for Intel router (only for old version)
    DB.WriteGlobalRoute(current_node, current_node.name+"_GlobalRoute_"+std::to_string(lidx)+".json", opath);

    // Detail Routing
    save_state( DB, current_node, lidx, opath, ".pre_dr", "Checkpoint : detail route", skip_saving_state);
    curr_route.RouteWork(1, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 0, 6, binary_directory, rate);
    save_state( DB, current_node, lidx, opath, ".post_dr", "Checkpoint : after detail route", skip_saving_state);
  }

  DB.WriteJSON (current_node, true, true, false, false, current_node.name+"_DR_"+std::to_string(lidx), drcInfo, opath);
  //DB.WriteJSON_Routability_Analysis (current_node, opath, const_cast<PnRDB::Drc_info&>(drcInfo));

  //double worst=0;
  //double th = 0.1;
  //bool Power_mesh_optimize = 0;
  rate = 0.1;

  if(current_node.isTop){
    save_state( DB, current_node, lidx, opath, ".pre_pg", "Checkpoint : Starting Power Grid Creation", skip_saving_state);

/*  DC Power Grid Simulation
    while(Power_mesh_optimize and worst < th and rate<1){

      curr_route.RouteWork(2, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 5, 6, binary_directory, rate);
    
      std::cout<<"Start MNA "<<std::endl;
      MNASimulation Test_MNA(current_node, const_cast<PnRDB::Drc_info&>(drcInfo));
   
      worst = Test_MNA.Return_Worst_Voltage();
      std::cout<<"worst voltage is "<< worst << std::endl;
      std::cout<<"End MNA "<<std::endl;
      Test_MNA.Clear_Power_Grid(current_node.Vdd);
      Test_MNA.Clear_Power_Grid(current_node.Gnd);      
      rate = rate + 0.1;
       
    }
*/
    curr_route.RouteWork(2, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 5, 6, binary_directory, rate);

    save_state( DB, current_node, lidx, opath, ".post_pg", "Checkpoint : End Power Grid Creation", skip_saving_state);

    DB.WriteJSON (current_node, true, true, false, true, current_node.name+"_PG_"+std::to_string(lidx), drcInfo, opath);
        
    std::cout<<"Checkpoint : Starting Power Routing"<<std::endl;
    save_state( DB, current_node, lidx, opath, ".pre_pr", "Checkpoint : Starting Power Routing", skip_saving_state);
    curr_route.RouteWork(3, current_node, const_cast<PnRDB::Drc_info&>(drcInfo), 0, 6, binary_directory, rate);
    save_state( DB, current_node, lidx, opath, ".post_pr", "Checkpoint : End Power Routing", skip_saving_state);

    DB.WriteJSON (current_node, true, false, true, true, current_node.name+"_PR_"+std::to_string(lidx), drcInfo, opath);
    DB.Write_Router_Report(current_node,opath);
        
  }

  DB.WriteJSON (current_node, true, true, true, true, current_node.name+"_"+std::to_string(lidx), drcInfo, opath);
  std::cout<<"Check point : before checkin\n";
  DB.PrintHierNode(current_node);
      
  save_state( DB, current_node, lidx, opath, "", "Final result", skip_saving_state);

  DB.WriteLef(current_node, current_node.name+"_"+std::to_string(lidx)+".lef", opath);

  /*
  std::ostringstream oss;
  oss << "gen_viewer_json.py"
      << " -b " << current_node.name
      << " -v " << lidx
      << " -d " << opath
      << " -o " << opath;
  string cmd(oss.str());

  int rc = system( cmd.c_str());
  std::cout << "System call to: \"" << cmd << "\" returned " << rc << std::endl;
  
  PnRDB::hierNode current_node2;

  DB.ReadDBJSON( current_node2,ofn);
  DB.WriteDBJSON( current_node2,ofn+"2");
  */

}

void static route_top_down(PnRdatabase& DB, const PnRDB::Drc_info& drcInfo, PnRDB::bbox bounding_box, PnRDB::Omark current_node_ort,
                           int idx, int& new_currentnode_idx, int lidx, const string& opath, const string& binary_directory,
                           bool skip_saving_state, bool adr_mode) {
  /*
  recursively DFS hiertree
  Inputs:
    current_node_ort: absolute orientation of current_node
    idx: index of current_node type
    new_currentnode_idx: index of current_node copy in hiertree, after routing
    lidx: number of layouts, now is one.
  */
  // 1.copy current_node from hiertree[idx]
  PnRDB::hierNode current_node = DB.hierTree[idx];
  string current_node_name = current_node.name;
  current_node.LL = bounding_box.LL;
  current_node.UR = bounding_box.UR;
  // 2.transform (translate and rotate) all points and rects of current_node into topnode coordinate;
  DB.TransformNode(current_node, current_node.LL, current_node_ort, PnRDB::TransformType::Forward);
  for (unsigned int bit = 0; bit < current_node.Blocks.size(); bit++) {
    if (current_node.Blocks[bit].child == -1) continue;
    int child_idx = current_node.Blocks[bit].child;
    // calculate childnode's orientation in topnode
    PnRDB::Omark childnode_orient =
        DB.RelOrt2AbsOrt(current_node_ort, current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].orient);
    string child_node_name = DB.hierTree[child_idx].name;
    // 3.childnode.LL and UR, absolute bouding box in topnode coordinate
    PnRDB::bbox childnode_box(current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].placedBox.LL,
                              current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].placedBox.UR);
    // 4.complete all children of current_node recursively
    int new_childnode_idx = 0;
    for (unsigned int lidx = 0; lidx < DB.hierTree[child_idx].numPlacement; lidx++) {
      route_top_down(DB, drcInfo, childnode_box, childnode_orient, child_idx, new_childnode_idx, lidx, opath, binary_directory,
                     skip_saving_state, adr_mode);
    }
    // 6.update current_node.blocks[i].intermetal/via/blockpin, absolute position and rect
    DB.CheckinChildnodetoBlock(current_node, bit, DB.hierTree[new_childnode_idx]);  // check in childnode into block

    // 7.current_node.Blocks[bit].child links to childnode
    current_node.Blocks[bit].child = new_childnode_idx;
  }
  // route_single_variant(DB, drcInfo, current_node, lidx, opath, binary_directory, skip_saving_state, adr_mode);

  // 8.transform (translate and rotate) current_node into current_node coordinate
  // undo transform current_node.LL and current_node_ort
  DB.TransformNode(current_node, current_node.LL, current_node_ort, PnRDB::TransformType::Backward);

  // 9.pushback current_node into hiertree, update current_node copy's index
  // update hiertree[blocks.*.child].parent = new_currentnode_idx
  DB.hierTree.push_back(current_node);
  new_currentnode_idx = DB.hierTree.size() - 1;
  for (unsigned int bit = 0; bit < current_node.Blocks.size(); bit++) {
    if (current_node.Blocks[bit].child == -1) continue;
    DB.hierTree[current_node.Blocks[bit].child].parent[0] = new_currentnode_idx;
  }
  return;

  /**
  string current_node_name = current_node.name;
  for (unsigned int bit = 0; bit < current_node.Blocks.size(); bit++) {
    if (current_node.Blocks[bit].child == -1) continue;
    int idx = current_node.Blocks[bit].child;
    // get and update childnode from parent
    // 1.copy childnode of current_node.blocks[i]
    PnRDB::hierNode childnode = DB.hierTree[idx];
    // calculate childnode's orientation in topnode
    PnRDB::Omark childnode_orient =
        DB.RelOrt2AbsOrt(current_node_ort, current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].orient);
    string child_node_name = childnode.name;

    // 2.childnode.LL = current_node.LL + block[i].placed.LL, orient = blocks[i].orient
    childnode.LL = current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].placedBox.LL;//  +current_node.LL;
    childnode.UR = current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].placedBox.UR;//  +current_node.LL;

    // 3.transform (translate and rotate) all points and rects of childnode into topnode coordinate;
    DB.TransformNode(childnode, childnode.LL, childnode_orient, PnRDB::TransformType::Forward);
    // all rects and points in childnode rotate by current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].orient
    // and translate by (childnode.LL.x, childnode.LL.y)

    // 4.complete all children of childnode recursively
    for (unsigned int lidx = 0; lidx < childnode.numPlacement; lidx++) {
      route_top_down(DB, drcInfo, childnode, childnode_orient, idx, lidx, opath, binary_directory, skip_saving_state, adr_mode);
    }

    // 5.transform (shift only) all points and rects of childnode into current_node coordinate
    // all rects and points in childnode translate by (-current_node.LL.x, -current_node.LL.y) -> into current_node coord;
    //DB.TranslateNode(childnode, current_node.LL);

    // 6.update current_node.blocks[i].intermetal/via/blockpin, absolute position and rect
    DB.CheckinChildnodetoBlock(idx, bit, childnode);  // check in childnode into block

    // 7.transform (translate and rotate) childnode into childnode coordinate
    // undo transform current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].placedBox.LL
    // and current_node.Blocks[bit].instance[current_node.Blocks[bit].selectedInstance].orient

    // 8.pushback childnode into hiertree
    // hiertree.push_back(childnode);

    // 9.current_node.Blocks[bit].child links to childnode
    // current_node.Blocks[bit].child=hiertree.size();
  }
  // route_single_variant(DB, drcInfo, current_node, lidx, opath, binary_directory, skip_saving_state, adr_mode);
  // DB.CheckinHierNode(parent, current_node);//check in current node into hiertree
  **/
}

int main(int argc, char** argv ){

  //
  // Enable or disable state saving in json at intermediate points
  // Currently adds 4 seconds to a 29 second baseline for the switched_capacitor_filter
  // And generates 69MB in files
  bool skip_saving_state = getenv( "PNRDB_SAVE_STATE") == NULL;
  bool adr_mode = getenv( "PNRDB_ADR_MODE") != NULL;
  bool disable_io = getenv( "PNRDB_disable_io") != NULL;; //turn off window outputs
  bool multi_thread = getenv( "PNRDB_multi_thread") != NULL;;  // run multi layouts in multi threads
  //bool disable_io = false; //turn off window outputs
  //bool multi_thread = false;  // run multi layouts in multi threads

  string opath="./Results/";
  string fpath=argv[1];
  string lfile=argv[2];
  string vfile=argv[3];
  string mfile=argv[4];
  string dfile=argv[5];
  string topcell=argv[6];
  int numLayout=std::stoi(argv[7]);
  int effort=std::stoi(argv[8]);
  if(fpath.back()=='/') {fpath.erase(fpath.end()-1);}
  if(opath.back()!='/') {opath+="/";}

  // Following codes try to get the path of binary codes
  string binary_directory = argv[0];
  cout <<"argv[0]: "<<binary_directory <<endl;
  int beginIdx = binary_directory.rfind('/');//find the last slash
  string str_lastOne = binary_directory.substr(beginIdx+1);
  cout <<"string after last slash: "<<str_lastOne <<endl;
  binary_directory = binary_directory.erase(beginIdx+1);
  cout <<"binary_directory: "<< binary_directory <<endl;

  mkdir(opath.c_str(), 0777);

  PnRdatabase DB(fpath, topcell, vfile, lfile, mfile, dfile); // construction of database
  PnRDB::Drc_info drcInfo=DB.getDrc_info();
  map<string, PnRDB::lefMacro> lefData = DB.checkoutSingleLEF();


  if ( !skip_saving_state) {
    queue<int> Q=DB.TraverseHierTree(); // traverse hierarchical tree in topological order
    json jsonStrAry = json::array();
    std::ofstream jsonStream;
    jsonStream.open( opath + "__hierTree.json");
    while (!Q.empty()) {
      jsonStrAry.push_back( DB.CheckoutHierNode(Q.front()).name);
      Q.pop();
    }
    jsonStream << std::setw(4) << jsonStrAry;
    jsonStream.close();
  }

  queue<int> Q=DB.TraverseHierTree(); // traverse hierarchical tree in topological order
  std::vector<int> TraverseOrder; //save traverse order, same as Q
  int Q_size = Q.size();
  for (int i = 0; i < Q_size;i++)
  {//copy Q to TraverseOrder
    TraverseOrder.push_back(Q.front());
    Q.pop();
    Q.push(TraverseOrder.back());
  }

  for (int i = 0; i < Q_size;i++)
  {
    int idx=TraverseOrder[i];
    cout<<"Main-Info: start to work on node "<<idx<<endl;
    if(disable_io)std::cout.setstate(std::ios_base::failbit);
    PnRDB::hierNode current_node=DB.CheckoutHierNode(idx);
    DB.PrintHierNode(current_node);

    
    save_state( DB, current_node, -1, opath, ".pre_prc", "Placer_Router_Cap", skip_saving_state);
    DB.AddingPowerPins(current_node);
    Placer_Router_Cap PRC(opath, fpath, current_node, drcInfo, lefData, 1, 6); //dummy, aspect ratio, number of aspect retio
    save_state( DB, current_node, -1, opath, ".post_prc", "Placer_Router_Cap", skip_saving_state);

    std::cout<<"Checkpoint : before place"<<std::endl;
    DB.PrintHierNode(current_node);

    save_state( DB, current_node, -1, opath, ".pre_pl", "Before Placement", skip_saving_state);
    
    // Placement
    std::vector<PnRDB::hierNode> nodeVec(numLayout, current_node);
    Placer curr_plc(nodeVec, opath, effort, const_cast<PnRDB::Drc_info&>(drcInfo)); // do placement and update data in current node

    for(unsigned int lidx=0; lidx<nodeVec.size(); ++lidx) {
      save_state( DB, nodeVec[lidx], lidx, opath, ".post_pl", "End Placement", skip_saving_state);
    }

    std::cout<<"Checkpoint: generated "<<nodeVec.size()<<" placements\n";
    /**
    if(multi_thread){
      std::thread t[nodeVec.size()];
      for(unsigned int lidx=0; lidx<nodeVec.size(); ++lidx) {
        t[lidx] = std::thread(route_single_variant, std::ref(DB), std::ref(drcInfo), std::ref(nodeVec[lidx]), lidx, 
                              std::ref(opath), std::ref(binary_directory), skip_saving_state, adr_mode);
      }
      for(unsigned int lidx=0; lidx<nodeVec.size(); ++lidx) {
        t[lidx].join();
      }
    }else{
      for(unsigned int lidx=0; lidx<nodeVec.size(); ++lidx) {
        route_single_variant( DB, drcInfo, nodeVec[lidx], lidx, opath, binary_directory, skip_saving_state, adr_mode);
      }
    }
    **/
    for(unsigned int lidx=0; lidx<nodeVec.size(); ++lidx) {
      DB.CheckinHierNode(idx, nodeVec[lidx]);
    }
    DB.hierTree[idx].numPlacement = nodeVec.size();

    //TreeVec[idx] = nodeVec;
    //Q.pop();
    if(disable_io)std::cout.clear();
    cout<<"Main-Info: complete node "<<idx<<endl;
  }

  int new_topnode_idx = 0;
  for (unsigned int lidx = 0; lidx < DB.hierTree[Q.back()].numPlacement; lidx++) {
    route_top_down(DB, drcInfo, PnRDB::bbox(DB.hierTree[Q.back()].LL, DB.hierTree[Q.back()].UR), PnRDB::N, Q.back(), new_topnode_idx, lidx,
                   opath, binary_directory, skip_saving_state, adr_mode);
  }

  /**
  for (int i = 0; i < Q_size; i++)
  {
    int idx = TraverseOrder[i];
    cout << "Main-Info: start to work on node " << idx << endl;
    if (disable_io)
      std::cout.setstate(std::ios_base::failbit);

    std::vector<PnRDB::hierNode> nodeVec = DB.CheckoutHierNodeVec(idx);
    //std::vector<PnRDB::hierNode> nodeVec = TreeVec[idx];
    if (multi_thread)
    {
      std::thread t[nodeVec.size()];
      for (unsigned int lidx = 0; lidx < nodeVec.size(); ++lidx)
      {
        t[lidx] = std::thread(route_single_variant, std::ref(DB), std::ref(drcInfo), std::ref(nodeVec[lidx]), lidx,
                              std::ref(opath), std::ref(binary_directory), skip_saving_state, adr_mode);
      }
      for (unsigned int lidx = 0; lidx < nodeVec.size(); ++lidx)
      {
        t[lidx].join();
      }
    }
    else
    {
      for (unsigned int lidx = 0; lidx < nodeVec.size(); ++lidx)
      {
        route_single_variant(DB, drcInfo, nodeVec[lidx], lidx, opath, binary_directory, skip_saving_state, adr_mode);
      }
    }

    for (unsigned int lidx = 0; lidx < nodeVec.size(); ++lidx)
    {
      DB.CheckinHierNode(idx, nodeVec[lidx]);
    }

    //Q.pop();
    if (disable_io)
      std::cout.clear();
    cout << "Main-Info: complete node " << idx << endl;
  }**/

  return 0;
}
