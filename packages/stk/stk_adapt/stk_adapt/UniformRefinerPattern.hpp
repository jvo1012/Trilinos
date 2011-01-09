#ifndef stk_adapt_UniformRefinerPattern_hpp
#define stk_adapt_UniformRefinerPattern_hpp

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <cmath>
#include <math.h>
#include <stk_mesh/base/Types.hpp>

#include "Teuchos_RCP.hpp"

#include <stk_percept/stk_mesh.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/array.hpp>
#include <boost/multi_array.hpp>


#include <Shards_BasicTopologies.hpp>
#include <Shards_CellTopologyData.h>

#include <Intrepid_CellTools.hpp>

#include <stk_percept/Util.hpp>
#include <stk_percept/PerceptMesh.hpp>
#include <stk_adapt/NodeRegistry.hpp>
#include <stk_percept/function/FieldFunction.hpp>

#include <stk_percept/math/Math.hpp>


#include <stk_adapt/sierra_element/RefinementTopology.hpp>
#include <stk_adapt/sierra_element/StdMeshObjTopologies.hpp>

#define TRACE_STAGE_PRINT_ON 0
#define TRACE_STAGE_PRINT (TRACE_STAGE_PRINT_ON && (m_eMesh.getRank()==0))

#if TRACE_STAGE_PRINT_ON
#  define TRACE_PRINT(a) do { trace_print(a); } while(0)
#  define TRACE_CPU_TIME_AND_MEM_0(a) do { Util::trace_cpu_time_and_mem_0(a); } while(0)
#  define TRACE_CPU_TIME_AND_MEM_1(a) do { Util::trace_cpu_time_and_mem_1(a); } while(0)
#else
#  define TRACE_PRINT(a) do {} while(0)
#  define TRACE_CPU_TIME_AND_MEM_0(a) do { } while(0)
#  define TRACE_CPU_TIME_AND_MEM_1(a) do { } while(0)
#endif

namespace stk {
  namespace adapt {

    using std::vector;
    using namespace stk::mesh;
    using namespace stk::percept;

    typedef std::vector<std::vector<std::string> > BlockNamesType;
    typedef std::map<std::string, std::string> StringStringMap;

    typedef vector<vector<vector<EntityId> > > NewSubEntityNodesType;
    typedef Elem::StdMeshObjTopologies::RefTopoX RefTopoX;

    typedef Elem::StdMeshObjTopologies::RefinementTopologyExtraEntry *RefTopoX_arr;

    // useful tools
#define NODE_COORD(node) stk::mesh::field_data( *eMesh.getCoordinatesField() , node )
#define VERT_COORD(ivert) NODE_COORD(*elem_nodes[ivert].entity())
#define EDGE_COORD(iedge,inode) NODE_COORD(*elem_nodes[cell_topo_data->edge[iedge].node[inode]].entity())
#define FACE_COORD(iface,inode) NODE_COORD(*elem_nodes[cell_topo_data->side[iface].node[inode]].entity())

    /// 2D array new_sub_entity_nodes[entity_rank][ordinal_of_node_on_sub_dim_entity]

#define VERT_N(i) elem_nodes[i].entity()->identifier()
#define EDGE_N(i) new_sub_entity_nodes[mesh::Edge][i][0]
#define FACE_N(i) new_sub_entity_nodes[mesh::Face][i][0]
#define NN(i_entity_rank, j_ordinal_on_subDim_entity) new_sub_entity_nodes[i_entity_rank][j_ordinal_on_subDim_entity][0]

#define EDGE_N_Q(iedge, inode_on_edge) new_sub_entity_nodes[mesh::Edge][iedge][inode_on_edge]
#define FACE_N_Q(iface, inode_on_face) new_sub_entity_nodes[mesh::Face][iface][inode_on_face]
#define NN_Q(i_entity_rank, j_ordinal_of_entity, k_ordinal_of_node_on_entity) \
    new_sub_entity_nodes[i_entity_rank][j_ordinal_of_entity][k_ordinal_of_node_on_entity]

#define NN_Q_P(i_entity_rank, j_ordinal_of_entity, k_ordinal_of_node_on_entity, perm) \
    new_sub_entity_nodes[i_entity_rank][j_ordinal_of_entity][perm[k_ordinal_of_node_on_entity]]

    struct SierraPort {};

    /// The base class for all refinement patterns
    /// ------------------------------------------------------------------------------------------------------------------------
    //template< typename ToTopology >
    class UniformRefinerPatternBase
    {
    protected:
      std::string m_fromTopoPartName;
      std::string m_toTopoPartName;
      mesh::PartVector m_fromParts;
      mesh::PartVector m_toParts;
      const std::string m_appendConvertString; //="_urpconv_"
      const std::string m_appendOriginalString; //="_urporig_100000"
      const std::string m_oldElementsPartName;
      EntityRank m_primaryEntityRank;
    public:
      //typedef ToTopology TTopo;

      UniformRefinerPatternBase() : m_appendConvertString("_urpconv"),
                                    m_appendOriginalString(percept::PerceptMesh::s_omit_part+"_1000"),  // _100000
                                    m_oldElementsPartName("urp_oldElements"),
                                    m_primaryEntityRank(mesh::Element)
      {
        Elem::StdMeshObjTopologies::bootstrap();

      }
      virtual ~UniformRefinerPatternBase() {}

      // deprecated
      virtual void doBreak()=0;

      virtual unsigned getFromType()=0;

      EntityRank getPrimaryEntityRank() { return m_primaryEntityRank; }
      /// must be provided by derived classes
      /// ------------------------------------------------------------------------------------------------------------------------

      /// supplies the ranks of the sub entities needed during refinement (eg. mesh::Face, mesh::Edge,..)
      /// 10/02/10 and the number of nodes needed for each sub entity
      virtual void fillNeededEntities(std::vector<NeededEntityType>& needed_entities)=0;

      /// supply the number of new elements per element during refinement
      virtual unsigned getNumNewElemPerElem()=0;

      /// given the node database (NodeRegistry), and the newly created nodes, and an iterator for the elements in the element pool,
      ///   create all new sub-elements of the refined element
      virtual void
      createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                        Entity& element,  NewSubEntityNodesType& new_sub_entity_nodes, vector<Entity *>::iterator& element_pool,
                        FieldBase *proc_rank_field=0)=0;

      /// optionally overridden (must be overridden if sidesets are to work properly) to provide info on which sub pattern
      /// should be used to refine side sets (and edge sets)
      virtual void setSubPatterns( std::vector<UniformRefinerPatternBase *>& bp, percept::PerceptMesh& eMesh )
      {
        /// default is only this pattern
        bp = std::vector<UniformRefinerPatternBase *>(1u, 0);
        bp[0] = this;
      }

      /// for i/o to work properly, supply string replacements such as for hex-->tet breaking, you would supply "quad"-->"tri" etc. string maps
      virtual StringStringMap fixSurfaceAndEdgeSetNamesMap()
      {
        // provide a null implementation
        StringStringMap map;
        return map;
      }

      /// provided by this class
      /// ------------------------------------------------------------------------------------------------------------------------

      /// deprecated
      const std::string& getFromTopoPartName() { return m_fromTopoPartName; }
      const std::string& getToTopoPartName() { return m_toTopoPartName; }

      mesh::PartVector& getToParts() { return m_toParts; }
      mesh::PartVector& getFromParts() { return m_fromParts; }
      const std::string& getAppendConvertString() { return m_appendConvertString; }
      const std::string& getAppendOriginalString() { return m_appendOriginalString; }
      const std::string& getOldElementsPartName() { return m_oldElementsPartName; }

      /// utilities
      /// ---------
      
      /// sets the needed number of nodes on each sub-entity to 1 - this is just a helper - in general, edges and faces have 1 new node
      /// for linear elements, and multiple new nodes in the case of quadratic elements
      void setToOne(std::vector<NeededEntityType>& needed_entities)
      {
        for (unsigned i = 0; i < needed_entities.size(); i++)
          {
            needed_entities[i].second = 1u;
          }
      }
      double * midPoint(const double *p1, const double *p2, int spatialDim, double *x)
      {
        x[0] = 0.5*(p1[0]+p2[0]);
        x[1] = 0.5*(p1[1]+p2[1]);
        if (spatialDim == 3)
          x[2] = 0.5*(p1[2]+p2[2]);
        return x;
      }

      double * getCentroid( double* pts[], int len, int spatialDim, double *x)
      {
        double dlen = double(len);
        for (int jsp = 0; jsp < spatialDim; jsp++)
          {
            x[jsp] = 0.0;
          }
        for (int ipt = 0; ipt < len; ipt++)
          {
            for (int jsp = 0; jsp < spatialDim; jsp++)
              {
                x[jsp] += pts[ipt][jsp] / dlen;
              }
          }
        return x;
      }

      static Teuchos::RCP<UniformRefinerPatternBase>
      createPattern(std::string refine, std::string enrich, std::string convert, percept::PerceptMesh& eMesh, BlockNamesType& block_names);

      static std::string s_convert_options;
      static std::string s_refine_options;
      static std::string s_enrich_options;



    };

    /// Utility intermediate base class providing more support for standard refinement operations
    /// ------------------------------------------------------------------------------------------------------------------------

    template< typename FTopo, typename TTopo  >
    class URP1
    {
    public:
      typedef FTopo FromTopology ;
      typedef TTopo ToTopology ;
    };

    template<typename FromTopology,  typename ToTopology >
    class URP :  public UniformRefinerPatternBase, public URP1<FromTopology, ToTopology>
    {
    public:

      enum
        {
          fromTopoKey    = FromTopology::key,
          toTopoKey      = ToTopology::key,
          topo_key_hex27 = shards::Hexahedron<27>::key,
          topo_key_quad9 = shards::Quadrilateral<9>::key,
          centroid_node  = (toTopoKey == topo_key_quad9 ? 8 :
                            (toTopoKey == topo_key_hex27 ? 20 : 0)
                            )
        };

      // return the type of element this pattern can refine
      virtual unsigned getFromType() { return fromTopoKey; }

      // draw
      /// draw a picture of the element's topology and its refinement pattern (using the "dot" program from AT&T's graphviz program)
      static std::string draw(bool showRefined = false)
      {
        Elem::StdMeshObjTopologies::bootstrap();

#define EXPRINT 0

        const CellTopologyData * const cell_topo_data = shards::getCellTopologyData<FromTopology>();
        CellTopology cell_topo(cell_topo_data);

        std::ostringstream graph_str;

        unsigned n_vert = FromTopology::vertex_count;
        unsigned n_node = FromTopology::node_count;
        unsigned n_edge = cell_topo.getEdgeCount();
        unsigned n_face = cell_topo.getFaceCount();
        unsigned n_side = cell_topo.getSideCount();

        graph_str <<
          "graph " << cell_topo.getName() << "  {\n"
                  << "#   name= " << cell_topo.getName() << "\n"
                  << "#   n_vert = " << n_vert << "\n"
                  << "#   n_node = " << n_node << "\n"
                  << "#   n_edge = " << n_edge << "\n"
                  << "#   n_face = " << n_face << "\n"
                  << "#   n_side = " << n_side << "\n"
                  <<
          "  ratio=1;\n"
          "  layout=nop;\n"
          "  size=\"4,4\";\n"
          "  bb=\"-50,-50,150,150\";\n"
          "  node [color=Green, fontcolor=Blue, font=Courier,  width=0.125,  height=0.125, shape=circle, fontsize=6, fixedsize=true, penwidth=0.2]; \n"
          //"  edge [style=dashed]; \n"
          "  edge [penwidth=0.1]; \n"
          ;


        //"  node [color=Green, fontcolor=Blue, font=Courier,  width=\"0.1\", height=\"0.1\", shape=none];\n" ;

        std::vector<NeededEntityType> needed_entities;

        Elem::CellTopology elem_celltopo = Elem::getCellTopology< FromTopology >();
        const Elem::RefinementTopology* ref_topo_p = Elem::getRefinementTopology(elem_celltopo);
        const Elem::RefinementTopology& ref_topo = *ref_topo_p;

        unsigned num_child = ref_topo.num_child();
        unsigned num_child_nodes = ref_topo.num_child_nodes();
        bool homogeneous_child = ref_topo.homogeneous_child();

        bool edge_exists[num_child_nodes][num_child_nodes];

        typedef Elem::StdMeshObjTopologies::RefTopoX RefTopoX;
        RefTopoX& ref_topo_x = Elem::StdMeshObjTopologies::RefinementTopologyExtra< FromTopology > ::refinement_topology;

        if (0) std::cout << num_child << homogeneous_child << n_vert;

        //if (n_face == 0) n_face = 1; // 2D face has one "face"
        if (0)
          std::cout << "tmp n_face= " << n_face << " n_side= " << n_side << std::endl;


        Math::Vector delta;
        double len_max = 0.0;
        for (unsigned i_edge = 0; i_edge < n_edge; i_edge++)
          {
            Math::Vector pc0( ref_topo_x[ cell_topo_data->edge[i_edge].node[0] ].parametric_coordinates );
            Math::Vector pc1( ref_topo_x[ cell_topo_data->edge[i_edge].node[1] ].parametric_coordinates );
            pc0 -= pc1;
            double len = norm_2(pc0);
            len_max = std::max(len_max, len);
          }

        double scv = 80.0;
        Math::Matrix scm = Math::scalingMatrix(scv / len_max);
        Math::Matrix rm = scm;

        Math::Vector centroid;
        for (unsigned i_node = 0; i_node < n_node; i_node++)
          {
            Math::Vector pc( ref_topo_x[i_node].parametric_coordinates );
            centroid += pc/(double(n_node));
          }
        if (0) std::cout << "len_max= " << len_max << " centroid= " << centroid << std::endl;

        if (cell_topo.getDimension() == 3)
          {

            // good one
            Math::Matrix rmx = Math::rotationMatrix(0, -60.0);
            Math::Matrix rmy = Math::rotationMatrix(1, 0.0);
            Math::Matrix rmz = Math::rotationMatrix(2, -30.0);

            //rm = ublas::prod(rmy, rmz);
            rm = ublas::prod(rmx, rmz);
            rm = ublas::prod(rmy, rm);
            rm = ublas::prod(scm, rm);
          }

        if (0)
          std::cout << rm;
        for (unsigned i_node = 0; i_node < n_node; i_node++)
          {
            double *pc = ref_topo_x[i_node].parametric_coordinates;
            Math::Vector v(pc);
            v -= centroid;
            v =  ublas::prod(rm, v);

            v(0) += scv/2.;
            v(1) += scv/2.;
            v(2) += scv/2.;

            graph_str << "  " << i_node << " [ pos=\"" << v(0) << "," << v(1) << "\"];\n";
          }

        // draw edges
        if (!showRefined)
        for (unsigned i_edge = 0; i_edge < n_edge; i_edge++)
          {
            unsigned nn = cell_topo_data->edge[i_edge].topology->vertex_count;
            for (unsigned j_node = 0; j_node < nn - 1; j_node++)
              {
                graph_str << "  "
                          << cell_topo_data->edge[i_edge].node[j_node] << " -- "
                          << cell_topo_data->edge[i_edge].node[(j_node + 1) % nn] << " ; \n" ;
              }
          }

        bool ft = (fromTopoKey == toTopoKey);

        if (showRefined && ft)
          {
            // draw edges
            for (unsigned i=0; i < num_child_nodes; i++)
              for (unsigned j = 0; j < num_child_nodes; j++)
                {
                  edge_exists[i][j]=false;
                }
            for (unsigned iChild = 0; iChild < num_child; iChild++)
              {
                // draw nodes
                for (unsigned jNode = 0; jNode < FromTopology::vertex_count; jNode++)
                  {
                    unsigned childNodeIdx = ref_topo.child_node(iChild)[jNode];
                    unsigned childNodeIdxCheck = ref_topo_x[childNodeIdx].ordinal_of_node;
                    VERIFY_OP(childNodeIdx, ==, childNodeIdxCheck, "childNodeIdxCheck");

                    double *pc = ref_topo_x[childNodeIdx].parametric_coordinates;
                    Math::Vector v(pc);
                    v -= centroid;
                    v =  ublas::prod(rm, v);

                    v(0) += scv/2.;
                    v(1) += scv/2.;
                    v(2) += scv/2.;

                    //graph_str << "  " << i_node << " [ pos=\"" << pc[0] << "," << pc[1] << "\"];\n";
                    std::string color="green";
                    if (childNodeIdx >= FromTopology::vertex_count)
                      color = "red";
                    graph_str << "  " << childNodeIdx << " [color=" << color << ", pos=\"" << v(0) << "," << v(1) << "\"];\n";

                  }

                for (unsigned i_edge = 0; i_edge < n_edge; i_edge++)
                  {
                    unsigned nn = cell_topo_data->edge[i_edge].topology->vertex_count;
                    for (unsigned j_node = 0; j_node < nn - 1; j_node++)
                      {
                        unsigned j0 = ref_topo.child_node(iChild)[ cell_topo_data->edge[i_edge].node[j_node] ];
                        unsigned j1 = ref_topo.child_node(iChild)[ cell_topo_data->edge[i_edge].node[(j_node + 1) % nn] ];
                        unsigned j00 = std::min(j0,j1);
                        unsigned j10 = std::max(j0,j1);
                        if (!edge_exists[j00][j10])
                          graph_str << "  " << std::min(j0,j1) << " -- " << std::max(j0,j1) << " ; \n" ;
                        edge_exists[j00][j10]=true;
                      }
                  }

              }
          }

        graph_str << "}\n";

        return std::string(graph_str.str());

      }

    protected:
      percept::PerceptMesh& m_eMesh;
      URP(percept::PerceptMesh& eMesh) : m_eMesh(eMesh) {}

      typedef ToTopology TTopo;
      typedef boost::array<unsigned, ToTopology::node_count > refined_element_type;


      void interpolateLine3(percept::PerceptMesh& eMesh, FieldBase* field,
                            MDArray& output_pts, Entity& element, MDArray& input_param_coords, double time_val=0.0)
      {
        int fieldStride = output_pts.dimension(1);
        unsigned *null_u = 0;

        mesh::PairIterRelation elem_nodes = element.relations(mesh::Node);
        double xi = input_param_coords(0, 0);

        // FIXME assumes {-1,0,1} element parametric coords
        double basis_val[3] = { (xi)*(xi - 1.0)/2.0, (1.0-xi)*(1.0+xi), (xi)*(1.0+xi)/2.0 };
        for (int i_stride=0; i_stride < fieldStride; i_stride++)
          {
            output_pts(0, i_stride) = 0.0;
          }
        for (unsigned i_node = 0; i_node < elem_nodes.size(); i_node++)
          {
            Entity *node = elem_nodes[i_node].entity();
            double *f_data = eMesh.field_data(field, *node, null_u);
            for (int i_stride=0; i_stride < fieldStride; i_stride++)
              {
                output_pts(0, i_stride) += f_data[i_stride]*basis_val[i_node];
              }
          }
      }

      /// helpers for interpolating fields, coordinates
      /// ------------------------------------------------------------------------------------------------------------------------
      //void interpolateFields(percept::PerceptMesh& eMesh, Entity& element, Entity& newElement,  refined_element_type& child_nodes,
#define EXTRA_PRINT_URP_IF 0
      void interpolateFields(percept::PerceptMesh& eMesh, Entity& element, Entity& newElement,  const unsigned *child_nodes,
                             RefTopoX_arr ref_topo_x, FieldBase *field)
      {
        EXCEPTWATCH;

        // FIXME
        //if (1) return;

        unsigned *null_u = 0;

        CellTopology cell_topo(get_cell_topology(element));

        // FIXME - need topo dimensions here
        int topoDim = cell_topo.getDimension();
        unsigned cell_topo_key = get_cell_topology(element)->key;

        static unsigned s_shell_line_2_key = shards::getCellTopologyData<ShellLine<2> >()-> key;
        static unsigned s_shell_line_3_key = shards::getCellTopologyData<ShellLine<3> >()-> key;
        static unsigned s_shell_tri_3_key = shards::getCellTopologyData<ShellTriangle<3> >()-> key;
        static unsigned s_shell_tri_6_key = shards::getCellTopologyData<ShellTriangle<6> >()-> key;
        static unsigned s_shell_quad_4_key = shards::getCellTopologyData<ShellQuadrilateral<4> >()-> key;
        static unsigned s_shell_quad_9_key = shards::getCellTopologyData<ShellQuadrilateral<9> >()-> key;
        if (cell_topo_key == s_shell_line_2_key || cell_topo_key == s_shell_line_3_key)
          {
            topoDim = 1;
          }

        if (cell_topo_key == s_shell_tri_3_key || cell_topo_key == s_shell_tri_6_key || 
            cell_topo_key == s_shell_quad_4_key || cell_topo_key == s_shell_quad_9_key)
          {
            topoDim = 2;
          }

        int fieldStride = 0;
        EntityRank fr_type = mesh::Node;
        if (EXTRA_PRINT_URP_IF) std::cout << "tmp field = " << field->name() << " topoDim= " << topoDim << std::endl;

        {
          unsigned nfr = field->restrictions().size();
          if (EXTRA_PRINT_URP_IF && nfr != 1 ) std::cout << "tmp P[" << 0 << "] info>    number of field restrictions= " << nfr << std::endl;
          for (unsigned ifr = 0; ifr < nfr; ifr++)
            {
              const FieldRestriction& fr = field->restrictions()[ifr];
              fr_type = fr.type();
              fieldStride = fr.stride[0] ;
              mesh::Part& frpart = eMesh.getMetaData()->get_part(fr.ordinal());
              if (EXTRA_PRINT_URP_IF && nfr != 1 ) std::cout << "tmp P[" << 0 << "] info>    number of field restrictions= " << nfr << " fr_type= " << fr_type
                                            << " fieldStride = " << fieldStride << " frpart= " << frpart.name()
                                            << std::endl;
            }
          {
            const stk::mesh::FieldBase::Restriction & r =
              field->restriction(fr_type, field->mesh_meta_data().universal_part());
            fieldStride = r.stride[0];
            if (EXTRA_PRINT_URP_IF) std::cout << "tmp stride = " <<  r.stride[0] << " fieldStride= " << fieldStride
                             << " fr_type= " << fr_type << " mesh::Element= " << mesh::Element<< std::endl;
          }
        }
        // FIXME
        if (!fieldStride || fr_type != mesh::Node)
          return;

        //FieldFunction field_func("tmp", field, eMesh, topoDim, fieldStride);
        FieldFunction field_func("tmp", field, eMesh, topoDim, fieldStride);

        MDArray input_pts(1, topoDim);
        MDArray input_param_coords(1, topoDim);
        MDArray output_pts(1, fieldStride);

        if (EXTRA_PRINT_URP_IF) std::cout << "tmp field = " << field->name() << " topoDim= " << topoDim << " fieldStride= " << fieldStride << std::endl;

        mesh::PairIterRelation new_elem_nodes = newElement.relations(mesh::Node);
        for (unsigned i_new_node = 0; i_new_node < new_elem_nodes.size(); i_new_node++)
          {
            unsigned childNodeIdx = child_nodes[i_new_node];
            if (EXTRA_PRINT_URP_IF) std::cout << "tmp childNodeIdx, i_new_node= " << childNodeIdx << " " << i_new_node << std::endl;
            double *param_coord = ref_topo_x[childNodeIdx].parametric_coordinates;
            if (EXTRA_PRINT_URP_IF) std::cout << "tmp childNodeIdx, i_new_node= " << childNodeIdx << " " << i_new_node
                             << " param_coord= " << param_coord[0] << " " << param_coord[1] << std::endl;
            for (int ip=0; ip < topoDim; ip++)
              {
                input_param_coords(0, ip) = param_coord[ip];
              }
            if (EXTRA_PRINT_URP_IF) std::cout << "tmp input_param_coords= " << input_param_coords << " cell_topo= " << cell_topo << std::endl;

            double time_val=0.0;

            /// unfortunately, Intrepid doesn't support a quadratic Line<3> element

            if (cell_topo.getDimension() == 1 && cell_topo.getNodeCount() == 3)  // Line<3> element
              {
                interpolateLine3(eMesh, field, output_pts, element, input_param_coords, time_val);
              }
            else
              {
                if (EXTRA_PRINT_URP_IF) std::cout << "tmp here3= " <<  std::endl;
                field_func(input_pts, output_pts, element, input_param_coords, time_val);
                if (EXTRA_PRINT_URP_IF) std::cout << "tmp here3a= " <<  std::endl;
              }
            if (EXTRA_PRINT_URP_IF) std::cout << "tmp here4= " <<  std::endl;

            Entity *new_node = new_elem_nodes[i_new_node].entity();

            {
              double *f_data_new = eMesh.field_data(field, *new_node, null_u);
              for (int ifd=0; ifd < fieldStride; ifd++)
                {
                  f_data_new[ifd] = output_pts(0, ifd);
                if (EXTRA_PRINT_URP_IF) std::cout << "tmp here5= " <<  std::endl;
                }
            }
          }
        if (EXTRA_PRINT_URP_IF) Util::printEntity(std::cout, newElement, eMesh.getCoordinatesField() );
      }

      /// do interpolation for all fields
      //     void interpolateFields(percept::PerceptMesh& eMesh, Entity& element, Entity& newElement, refined_element_type& child_nodes,
      void interpolateFields(percept::PerceptMesh& eMesh, Entity& element, Entity& newElement, const unsigned *child_nodes,
                             RefTopoX_arr ref_topo_x)
      {
        const FieldVector & fields = eMesh.getMetaData()->get_fields();
        unsigned nfields = fields.size();
        //std::cout << "P[" << p_rank << "] info>    Number of fields = " << fields.size() << std::endl;
        for (unsigned ifld = 0; ifld < nfields; ifld++)
          {
            FieldBase *field = fields[ifld];
            //std::cout << "P[" << eMesh.getRank() << "] field = " << field->name() << std::endl;

            interpolateFields(eMesh, element, newElement, child_nodes, ref_topo_x, field);
          }
      }

      enum { NumNewElements_Enrich = 1 };

      void
      genericEnrich_createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                                      Entity& element,  NewSubEntityNodesType& new_sub_entity_nodes, vector<Entity *>::iterator& element_pool,
                                      FieldBase *proc_rank_field=0)
      {
        std::vector<NeededEntityType> needed_entities;
        fillNeededEntities(needed_entities);

        const CellTopologyData * const cell_topo_data = get_cell_topology(element);

        typedef boost::array<unsigned, ToTopology::node_count > quadratic_type;

        static vector<quadratic_type> elems(NumNewElements_Enrich);

        CellTopology cell_topo(cell_topo_data);
        const mesh::PairIterRelation elem_nodes = element.relations(Node);

        std::vector<stk::mesh::Part*> add_parts;
        std::vector<stk::mesh::Part*> remove_parts;

        unsigned n_edges = cell_topo_data->edge_count;

        unsigned n_faces = cell_topo.getFaceCount();
        if (n_faces == 0) n_faces = 1; // 2D face has one "face"
        unsigned n_sides = cell_topo.getSideCount();
        if (0)
          std::cout << "tmp  n_faces= " << n_faces << " n_sides= " << n_sides << std::endl;

        add_parts = m_toParts;

        for (unsigned i_need = 0; i_need < needed_entities.size(); i_need++)
          {
            unsigned nSubDimEntities = 1;
            if (needed_entities[i_need].first == Edge)
              {
                nSubDimEntities = cell_topo_data->edge_count;
              }
            else if (needed_entities[i_need].first == Face)
              {
                nSubDimEntities = cell_topo_data->side_count;
              }

            for (unsigned iSubDim = 0; iSubDim < nSubDimEntities; iSubDim++)
              {
                nodeRegistry.makeCentroid(*const_cast<Entity *>(&element), needed_entities[i_need].first, iSubDim);
                nodeRegistry.addToExistingParts(*const_cast<Entity *>(&element), needed_entities[i_need].first, iSubDim);
                nodeRegistry.interpolateFields(*const_cast<Entity *>(&element), needed_entities[i_need].first, iSubDim);
              }
          }

        const CellTopologyData * const cell_topo_data_toTopo = shards::getCellTopologyData< ToTopology >();
        shards::CellTopology cellTopo(cell_topo_data_toTopo);

#define CENTROID_N NN(m_primaryEntityRank,0)

        quadratic_type& EN = elems[0];

        for (unsigned ind = 0; ind < FromTopology::vertex_count; ind++)
          {
            EN[ind] = VERT_N(ind);
          }

        for (unsigned i_need = 0; i_need < needed_entities.size(); i_need++)
          {
            if (needed_entities[i_need].first == Edge)
              {
                for (unsigned i_edge = 0; i_edge < n_edges; i_edge++)
                  {
                    unsigned edge_ord = cell_topo_data_toTopo->edge[i_edge].node[2];
                    unsigned inode = EDGE_N(i_edge);
                    EN[edge_ord] = inode;
                  }
              }
            else if (needed_entities[i_need].first == Face)
              {
                for (unsigned i_face = 0; i_face < n_faces; i_face++)
                  {
                    // FIXME assumes face is quadrilateral
                    unsigned face_ord = 0;
                    if (toTopoKey == topo_key_quad9)
                      face_ord = 8;
                    else
                      face_ord = cell_topo_data_toTopo->side[i_face].node[8];

                    unsigned inode = FACE_N(i_face);

                    //std::cout << "tmp P[" << eMesh.getRank() << "] inode = " << inode << " for i_face = " << i_face << " face_ord= " << face_ord << std::endl;

                    if (!inode)
                      {
                        std::cout << "P[" << eMesh.getRank() << "] inode = 0 for i_face = " << i_face << " face_ord= " << face_ord << std::endl;
                        //throw std::logic_error("UniformRefinerPatternBase::genericEnrich_createNewElements bad entity id = 0 ");
                      }

                    EN[face_ord] = inode;
                  }
              }
            else if (needed_entities[i_need].first == Element)
              {
                EN[ centroid_node ] = CENTROID_N;
              }
          }

#undef CENTROID_N

        for (unsigned ielem=0; ielem < elems.size(); ielem++)
          {
            Entity& newElement = *(*element_pool);

            // FIXME
            if (m_primaryEntityRank==Element &&  proc_rank_field)
              {
                double *fdata = stk::mesh::field_data( *static_cast<const ScalarFieldType *>(proc_rank_field) , newElement );
                fdata[0] = double(newElement.owner_rank());
              }

            change_entity_parts(eMesh, element, newElement);

            for (int inode=0; inode < ToTopology::node_count; inode++)
              {
                mesh::EntityId eid = elems[ielem][inode];
                if (!eid)
                  {
                    std::cout << "P[" << eMesh.getRank() << "] eid = 0 for inode = " << inode << std::endl;
                    throw std::logic_error("UniformRefinerPatternBase::genericEnrich_createNewElements bad entity id = 0 ");
                  }
                mesh::Entity& node = eMesh.createOrGetNode(eid);
                eMesh.getBulkData()->declare_relation(newElement, node, inode);
              }

            element_pool++;

          }

      }

      /*------------------------------------------------------------------------*/
      /*  comments from Shards_CellTopology.hpp with locally added comments
       * \brief  Find the permutation from the expected nodes to the actual nodes,
       *
       *  Find permutation 'p' such that:
       *    actual_node[j] == expected_node[ top.permutation[p].node[j] ]
       *  for all vertices.
       *
       *  So, actual_node[j] is the sub-dim cell; expected_node is the parent cell->subcell[dim][Ord].node[ perm[p][j] ]
       *
       *  Get sub-dim cell's nodes from NodeRegistry (actual_node array); or just sort them into a set
       *  Get parent element's sub-dim cell nodes from element->subcell[dim][ord].node
       *  Get permutation using findPermutation(cell_topo, parent->subcell[dim][ord].node, subdim_cell_sorted_nodes)
       *
       *
       *  Then <b> ParentCell.node(K) == SubCell.node(I) </b> where:
       *  -  SubCellTopology == ParentCellTopology->subcell[dim][Ord].topology
       *  -  K  = ParentCellTopology->subcell[dim][Ord].node[IP]
       *  -  IP = SubCellTopology->permutation[P].node[I]
       *  -  I  = SubCellTopology->permutation_inverse[P].node[IP]

      */

      int getPermutation(int num_verts, Entity& element, CellTopology& cell_topo, unsigned rank_of_subcell, unsigned ordinal_of_subcell)
      {
        //std::cout << "tmp element 0= " << element << " cell_topo= " << cell_topo.getName() << " rank_of_subcell= " << rank_of_subcell
        //<< " num_verts= " << num_verts
        //          << std::endl;

        if (rank_of_subcell == 0 || rank_of_subcell == 3) return 0;

        static std::set<unsigned> subdimCell_global_baseline;
        static std::vector<unsigned> subdimCell_global_baseline_vector;
        static std::set<unsigned>::iterator subdimCell_global_baseline_iter;
        static std::vector<unsigned> subCell_from_element;

        subdimCell_global_baseline.clear();
        subdimCell_global_baseline_vector.resize(0);
        subCell_from_element.resize(0);

        const mesh::PairIterRelation elem_nodes = element.relations(mesh::Node);

        const unsigned * inodes = cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].node;
        int num_subcell_verts = cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].topology->vertex_count;
        for (int iv = 0; iv < num_subcell_verts; iv++)
          {
            subdimCell_global_baseline.insert(elem_nodes[inodes[iv]].entity()->identifier());
            subCell_from_element.push_back(elem_nodes[inodes[iv]].entity()->identifier());
          }

        for (subdimCell_global_baseline_iter = subdimCell_global_baseline.begin();
             subdimCell_global_baseline_iter != subdimCell_global_baseline.end();
             subdimCell_global_baseline_iter++)
          {
            subdimCell_global_baseline_vector.push_back( *subdimCell_global_baseline_iter );
          }

        int perm = -1;
        
        /// for tri or quad faces we search for the min node, then look at its two neighbors along edges
        ///   - if the first edge is the next node in line (in terms of its id) we use the ordering as 0,1,2,3
        ///   else it is flipped and we reverse the ordering
        ///   - once the ordering is determined, the actual permutation can be deduced from shards
        ///

          {
            // quad or tri
            if (0 && num_subcell_verts==3)
              {
                std::cout << "tmp b4 element 1= " << element << " cell_topo= " << cell_topo.getName() 
                          << " rank_of_subcell= " << rank_of_subcell << std::endl;
                std::cout << "tmp b4 subdimCell_global_baseline_vector= " << subdimCell_global_baseline_vector << std::endl;
                std::cout << "tmp b4 subCell_from_element = " << subCell_from_element << std::endl;
              }

            subdimCell_global_baseline_iter = subdimCell_global_baseline.begin();
            unsigned i0 = *subdimCell_global_baseline_iter;
            int j0 = -1;
            for (int iv = 0; iv < num_subcell_verts; iv++)
              {
                if (i0 == subCell_from_element[iv])
                  {
                    j0 = iv;
                    break;
                  }
              }
            if (j0 < 0) throw std::logic_error("j0 < 0 ");
            int j1 = (j0 + 1) % num_subcell_verts;
            int j2 = (j0 + (num_subcell_verts-1)) % num_subcell_verts;  // adds 3 for quads, or 2 for tris to pickup the neigh node
            if (subCell_from_element[j1] < subCell_from_element[j2])
              {
                for (int iv = 0; iv < num_subcell_verts; iv++)
                  {
                    subdimCell_global_baseline_vector[iv] = subCell_from_element[(j0 + iv) % num_subcell_verts];
                  }
              }
            else
              {
                for (int iv = 0; iv < num_subcell_verts; iv++)
                  {
                    subdimCell_global_baseline_vector[(num_subcell_verts - iv) % num_subcell_verts] = 
                      subCell_from_element[(j0 + iv) % num_subcell_verts];
                  }
              }

            perm = findPermutation(cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].topology,
                                              &subdimCell_global_baseline_vector[0], &subCell_from_element[0]);

            if ( perm < 0)
              {
                std::cout << "tmp aft element 1= " << element << " cell_topo= " << cell_topo.getName() << " rank_of_subcell= " << rank_of_subcell << std::endl;
                std::cout << "tmp aft subdimCell_global_baseline_vector= " << subdimCell_global_baseline_vector << std::endl;
                std::cout << "tmp aft subCell_from_element = " << subCell_from_element << std::endl;
              }

            if (0 && num_subcell_verts==3)
              {
                const unsigned *perm_array = cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].topology->permutation[perm].node;
                for (int iv = 0; iv < num_subcell_verts; iv++)
                  {
                    std::cout << "tmp perm_array[" << iv << "]=  " << perm_array[iv] << std::endl;
                  }
              }

          }

        if (perm < 0)
          {
            std::cout << "tmp element 1= " << element << " cell_topo= " << cell_topo.getName() << " rank_of_subcell= " << rank_of_subcell << std::endl;
            std::cout << "tmp subdimCell_global_baseline_vector= " << subdimCell_global_baseline_vector << std::endl;
            std::cout << "tmp subCell_from_element = " << subCell_from_element << std::endl;
          }

        return perm;
      }

      //static std::vector<NeededEntityType> s_needed_entities;
      //static vector<refined_element_type> s_elems;
      //static std::vector<stk::mesh::Part*> s_add_parts;
      //static std::vector<stk::mesh::Part*> s_remove_parts;

      void
      genericRefine_createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                                      Entity& element,  NewSubEntityNodesType& new_sub_entity_nodes, vector<Entity *>::iterator& element_pool,
                                      FieldBase *proc_rank_field=0)
      {
        EXCEPTWATCH;
        static std::vector<NeededEntityType> needed_entities;
        fillNeededEntities(needed_entities);

        //

        // CHECK
        const CellTopologyData * const cell_topo_data = get_cell_topology(element);

        static vector<refined_element_type> elems;
        elems.resize(getNumNewElemPerElem());

        CellTopology cell_topo(cell_topo_data);
        bool linearElement = Util::isLinearElement(cell_topo);

        const mesh::PairIterRelation elem_nodes = element.relations(Node);

        unsigned cellDimension = cell_topo.getDimension();


        unsigned n_edges = cell_topo_data->edge_count;
        if (n_edges == 0) n_edges = 1; // 1D edge has one "edge"
        unsigned n_faces = cell_topo.getFaceCount();
        if (n_faces == 0) n_faces = 1; // 2D face has one "face"
        unsigned n_sides = cell_topo.getSideCount();
        if (0) std::cout << "tmp  n_edges= " << n_edges << " n_faces= " << n_faces << " n_sides= " << n_sides << std::endl;

        for (unsigned i_need = 0; i_need < needed_entities.size(); i_need++)
          {
            unsigned nSubDimEntities = 0;
            if (needed_entities[i_need].first == Edge)
              {
                nSubDimEntities = cell_topo_data->edge_count;
              }
            else if (needed_entities[i_need].first == Face)
              {
                nSubDimEntities = cell_topo_data->side_count;
              }
            else if (needed_entities[i_need].first == Element)
              {
                nSubDimEntities = 1;
              }

            // FIXME - assumes first node on each sub-dim entity is the "linear" one
            for (unsigned iSubDim = 0; iSubDim < nSubDimEntities; iSubDim++)
              {
                nodeRegistry.addToExistingParts(*const_cast<Entity *>(&element), needed_entities[i_need].first, iSubDim);
                if (linearElement)
                  {
                    nodeRegistry.interpolateFields(*const_cast<Entity *>(&element), needed_entities[i_need].first, iSubDim);
                  }
              }
          }

        const CellTopologyData * const cell_topo_data_toTopo = shards::getCellTopologyData< ToTopology >();
        shards::CellTopology cellTopo(cell_topo_data_toTopo);

        Elem::CellTopology elem_celltopo = Elem::getCellTopology< FromTopology >();
        const Elem::RefinementTopology* ref_topo_p = Elem::getRefinementTopology(elem_celltopo); // CHECK
        const Elem::RefinementTopology& ref_topo = *ref_topo_p;

        unsigned num_child = ref_topo.num_child();

        VERIFY_OP(num_child, == , getNumNewElemPerElem(), "genericRefine_createNewElements num_child problem");

        // FIXME check if this is a wedge
        //bool homogeneous_child = ref_topo.homogeneous_child();
        //VERIFY_OP(homogeneous_child, ==, true, "genericRefine_createNewElements homogeneous_child");

        RefTopoX& ref_topo_x = Elem::StdMeshObjTopologies::RefinementTopologyExtra< FromTopology > ::refinement_topology;  

        for (unsigned iChild = 0; iChild < num_child; iChild++)
          {
            refined_element_type& EN = elems[iChild];
            for (unsigned jNode = 0; jNode < ToTopology::node_count; jNode++)
              {
                unsigned childNodeIdx = ref_topo.child_node(iChild)[jNode];

                unsigned childNodeIdxCheck = ref_topo_x[childNodeIdx].ordinal_of_node;
                VERIFY_OP(childNodeIdx, ==, childNodeIdxCheck, "childNodeIdxCheck");

                unsigned inode=0;
                unsigned rank_of_subcell            = ref_topo_x[childNodeIdx].rank_of_subcell;
                unsigned ordinal_of_subcell         = ref_topo_x[childNodeIdx].ordinal_of_subcell;
                unsigned ordinal_of_node_on_subcell = ref_topo_x[childNodeIdx].ordinal_of_node_on_subcell;
                unsigned num_nodes_on_subcell       = ref_topo_x[childNodeIdx].num_nodes_on_subcell;

                // bool usePerm = true;  FIXME FIXME FIXME
                bool usePerm = false;
                const unsigned * perm_array = 0;
                if (usePerm)
                  {
                    int perm_ord = getPermutation(FromTopology::vertex_count, element, cell_topo, rank_of_subcell, ordinal_of_subcell);
                    if (perm_ord < 0)
                      throw std::logic_error("permutation < 0 ");
                    //std::cout << "tmp 0 " << perm_ord << " rank_of_subcell= " << rank_of_subcell << " ordinal_of_subcell= " << ordinal_of_subcell <<  std::endl;
                    //std::cout << "tmp 0 " << cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].topology << std::endl;
                    if (1 <= rank_of_subcell && rank_of_subcell <= 2)
                      {
                        perm_array = cell_topo.getTopology()->subcell[rank_of_subcell][ordinal_of_subcell].topology->permutation[perm_ord].node;
                      }

                    //std::cout << "tmp 1 " << std::endl;
                  }

                switch (rank_of_subcell)
                  {
                  case 0:
                    inode = VERT_N(ordinal_of_subcell);
                    break;
                  case 1:
                    if (usePerm) // FIXME
                      if (num_nodes_on_subcell > 1)
                        inode = NN_Q_P(mesh::Edge, ordinal_of_subcell, ordinal_of_node_on_subcell, perm_array);
                      else
                        inode = NN_Q(mesh::Edge, ordinal_of_subcell, ordinal_of_node_on_subcell);
                    else
                      inode = EDGE_N_Q(ordinal_of_subcell, ordinal_of_node_on_subcell);

                    break;
                  case 2:
                    //if (m_primaryEntityRank == mesh::Face)
                    if (cellDimension == 2)
                      {
                        VERIFY_OP(ordinal_of_subcell, == , 0, "createNewElements: ordinal_of_subcell");

                        if (usePerm)
                          {
                            if (num_nodes_on_subcell > 1)
                              {
                                inode = NN_Q_P(m_primaryEntityRank, ordinal_of_subcell, ordinal_of_node_on_subcell, perm_array);
                              }
                            else
                              {
                                inode = NN_Q(m_primaryEntityRank, ordinal_of_subcell, ordinal_of_node_on_subcell);
                              }
                          }
                        else
                          {
                            inode = NN_Q(m_primaryEntityRank, ordinal_of_subcell, ordinal_of_node_on_subcell);
                          }
                      }
                    else
                      {
                        if (usePerm)
                          {
                            if (num_nodes_on_subcell > 1)
                              {
                                inode = NN_Q_P(mesh::Face, ordinal_of_subcell, ordinal_of_node_on_subcell, perm_array);
                              }
                            else
                              {
                                inode = NN_Q(mesh::Face, ordinal_of_subcell, ordinal_of_node_on_subcell);
                              }
                          }
                        else
                          {
                            inode = NN_Q(mesh::Face, ordinal_of_subcell, ordinal_of_node_on_subcell);
                          }

                      }
                    break;
                  case 3:
                    inode = NN_Q(m_primaryEntityRank, ordinal_of_subcell, ordinal_of_node_on_subcell);
                    break;
                  default:
                    throw std::logic_error("UniformRefinerPattern logic error");
                  }

                if (0) std::cout << "tmp 2.1 " << inode << " " << jNode << " rank_of_subcell= " << rank_of_subcell
                                 << " ordinal_of_subcell = " << ordinal_of_subcell
                                 << " ordinal_of_node_on_subcell = " << ordinal_of_node_on_subcell
                                 << std::endl;

                EN[jNode] = inode;
              }
          }


        for (unsigned iChild = 0; iChild < num_child; iChild++)
          {
            Entity& newElement = *(*element_pool);

            // FIXME
            if (m_primaryEntityRank == Element &&  proc_rank_field)
              {
                //exit(1);  // FIXME FIXME FIXME CHECK
                double *fdata = stk::mesh::field_data( *static_cast<const ScalarFieldType *>(proc_rank_field) , newElement );
                fdata[0] = double(newElement.owner_rank());
              }

            // CHECK
            change_entity_parts(eMesh, element, newElement);

            for (int inode=0; inode < ToTopology::node_count; inode++)
              {
                mesh::EntityId eid = elems[iChild][inode];
                if (!eid)
                  {
                    std::cout << "P[" << eMesh.getRank() << "] eid = 0 for inode = " << inode << std::endl;
                    throw std::logic_error("UniformRefinerPatternBase::genericRefine_createNewElements bad entity id = 0 ");
                  }

                /**/                                                         TRACE_CPU_TIME_AND_MEM_0(CONNECT_LOCAL_URP_createOrGetNode);
                mesh::Entity& node = eMesh.createOrGetNode(eid);
                /**/                                                         TRACE_CPU_TIME_AND_MEM_1(CONNECT_LOCAL_URP_createOrGetNode);
               
                /**/                                                         TRACE_CPU_TIME_AND_MEM_0(CONNECT_LOCAL_URP_declare_relation);
                eMesh.getBulkData()->declare_relation(newElement, node, inode);
                //register_relation(newElement, node, inode);
                /**/                                                         TRACE_CPU_TIME_AND_MEM_1(CONNECT_LOCAL_URP_declare_relation);
              }

            if (!linearElement)
              {
                interpolateFields(eMesh, element, newElement, ref_topo.child_node(iChild),  &ref_topo_x[0], eMesh.getCoordinatesField() );
                interpolateFields(eMesh, element, newElement, ref_topo.child_node(iChild),  &ref_topo_x[0]);
              }

            element_pool++;
          }
      }

      /// utility methods for converting Sierra tables to new format (which groups DOF's on sub-entities)
      static bool on_parent_vertex(unsigned childNodeIdx)
      {
        return (childNodeIdx < FromTopology::vertex_count);
      }

      static bool on_parent_edge(unsigned childNodeIdx, const Elem::RefinementTopology& ref_topo)
      {
        unsigned num_child_nodes = ref_topo.num_child_nodes();

        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );
        unsigned n_edges = cell_topo.getEdgeCount();

        for (unsigned i_edge = 0; i_edge < n_edges; i_edge++)
          {
            const UInt *edge_nodes = ref_topo.edge_node(i_edge);

            unsigned i_ord = 0;
            for (unsigned i_edge_n = 0; edge_nodes[i_edge_n] != END_UINT_ARRAY; i_edge_n++)
              {
                unsigned j_e_node = edge_nodes[i_edge_n];
                if (childNodeIdx == j_e_node)
                  {
                    return true;
                  }
                if (j_e_node < num_child_nodes)
                  ++i_ord;
              }
          }
        return false;
      }


      static bool on_parent_face(unsigned childNodeIdx, const Elem::RefinementTopology& ref_topo)
      {
        unsigned num_child_nodes = ref_topo.num_child_nodes();

        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );
        if (cell_topo.getDimension() == 2)
          {
            return true; // by definition
          }

        unsigned n_faces = cell_topo.getFaceCount();
        if (n_faces == 0) n_faces = 1; // 2D face has one "face"

        for (unsigned i_face = 0; i_face < n_faces; i_face++)
          {
            const UInt *face_nodes = ref_topo.face_node(i_face);

            unsigned i_ord = 0;
            for (unsigned i_face_n = 0; face_nodes[i_face_n] != END_UINT_ARRAY; i_face_n++)
              {
                unsigned j_e_node = face_nodes[i_face_n];
                if (childNodeIdx == j_e_node)
                  {
                    return true;
                  }
                if (j_e_node < num_child_nodes)
                  ++i_ord;
              }
          }
        return false;
      }

      static bool on_parent_edge_interior(unsigned childNodeIdx, const Elem::RefinementTopology& ref_topo, unsigned& i_edge, unsigned& i_ord, unsigned& n_ord)
      {
        if (on_parent_vertex(childNodeIdx))
          return false;

        unsigned num_child_nodes = ref_topo.num_child_nodes();
        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );
        unsigned n_edges = cell_topo.getEdgeCount();
        if (n_edges == 0) n_edges = 1;

        for ( i_edge = 0; i_edge < n_edges; i_edge++)
          {
            const UInt *edge_nodes = ref_topo.edge_node(i_edge);

            n_ord = 0;
            int n_edge_n = 0;
            for (unsigned i_edge_n = 0; edge_nodes[i_edge_n] != END_UINT_ARRAY; i_edge_n++)
              {
                if (on_parent_vertex(edge_nodes[i_edge_n]))
                  continue;
                if (edge_nodes[i_edge_n] < num_child_nodes)
                  ++n_ord;
                ++n_edge_n;
              }

            i_ord = 0;
            for (unsigned i_edge_n = 0; edge_nodes[i_edge_n] != END_UINT_ARRAY; i_edge_n++)
              // go in reverse to put mid node at the end
              //for (int i_edge_n = n_edge_n-1; i_edge_n >= 0; i_edge_n--)
              {
                if (on_parent_vertex(edge_nodes[i_edge_n]))
                  continue;
                unsigned j_e_node = edge_nodes[i_edge_n];
                if (childNodeIdx == j_e_node)
                  {
                    if (i_ord == 0)
                      i_ord = n_ord-1;
                    else
                      --i_ord;

                    return true;
                  }
                if (j_e_node < num_child_nodes)
                  ++i_ord;
              }
          }
        return false;
      }

      /** SPECIAL CASE ALERT (see below for triangle faces)
       *
       *  To prepare for future truly hierarchical elements, we want to number the nodes on the interior of each face
       *  in a manner that mimics the parent element.  For example, nodes {8, 17-20, and 21-24} below are not numbered
       *  consistent with the parent quadratic element.  The consistent numbering would be {21-24, 17-20, 8} to correspond
       *  with parent's {0-8} numbering:
       *
       * After refinement:
       *
       *  3    14    6   13     2   CHILD 9-Node Quadrilateral Object Nodes
       *   o----*----o----*----o    (new nodes = *)
       *   |         |         |
       *   |   24    |    23   |
       * 15*    *    *19  *    *12
       *   |         |         |
       *   |        8|    18   |
       * 7 o----*----o----*----o 5
       *   |   20    |         |
       *   |         |         |
       * 16*    *  17*    *    *11
       *   |   21    |   22    |
       *   |         |         |
       *   o----*----o----*----o
       *  0     9    4   10     1
       *
       * The way this meshes with hierarchical elements is to imagine the face being
       *  part of a p-element with local polynomial degree 4x3 (p=4 in local-x of the face, p=3 in local-y).  In that
       *  case, we can think of the nodes laid out in a rectangular pattern as:
       *
       *      o---o---o
       *      |   |   |
       *      o---o---o
       *
       *  or in the parent's hierarchical element (e.g. node 5 is the hiearchical edge-bubble DOF), and numbered
       *  in a lexicographical (x-first, then y) ordering:
       *
       *  3        6        2
       *   o-------o-------o
       *   |               |
       *   |   11  12  13  |
       *   |    o--o--o    |
       * 7 o    |  |  |    o 5
       *   |    o--o--o    |
       *   |   8   9   10  |
       *   |               |
       *   o-------o-------o
       *  0        4        1
       *
       *
       *  Of course, this is only symbolic of the hiearchical DOF's at the center of the face, shown as Lagrange-type DOF's for
       *  exposition only.  In reality, we denote just with a + at the center:
       *
       *            6
       *   3        _       2
       *    o---------------o
       *    |               |
       *    |               |
       *    |         8     |
       * 7 ||       +       || 5
       *    |               |
       *    |               |
       *    |               |
       *    o---------------o
       *   0        -        1
       *            4
       *
       *  So, we renumber the nodes on the face interior as: {21-24, 17-20, 8} - this is used below to choose face DOF ordinals
       *  in building the tables in printRefinementTopoX_Table
       *
       */
      static unsigned renumber_quad_face_interior_nodes(unsigned original_node)
      {
        static int face_interior_inverse_map[] = { -1, /* 0 */
                                                        -1, -2, -3, -4, -5, -6, -7, 8, -9, -10,
                                                        -11, -12, -13, -14, -15, -16, 
                                                        4, 5, 6, 7, // -17, -18, -19, -20
                                                        0, 1, 2, 3 }; //-21, -22, -23, -24};

        /*
        static int face_interior_map[] = {21, 22, 23, 24, 
                                               17, 18, 19, 20,
                                               8 };
        */
        if (original_node >= 25) throw std::logic_error("renumber_quad_face_interior_nodes 1");
        int val = face_interior_inverse_map[original_node];
        if (val < 0) throw std::logic_error("renumber_quad_face_interior_nodes 2");
        return (unsigned)val;
      }

    /*--------------------------------------------------------------------*/
    /**
     *           2            PARENT 6-Node Triangle Object Nodes
     *           o
     *          / \
     *         /   \          (PARENT) 6-Node Triangle Object Edge Node Map:
     *        /     \
     *     5 o       o 4      { {0, 1, 3}, {1, 2, 4}, {2, 0, 5} };
     *      /         \
     *     /           \
     *    /             \
     *   o-------o-------o
     *  0        3        1
     *
     *   After refinement:
     *
     *           2            CHILD 6-Node Triangle Object Nodes
     *           o                  (new nodes = *)
     *          / \
     *      10 *   * 9
     *        / 14  \
     *     5 o---*---o 4
     *      / \     / \
     *  11 * 12*   *13 * 8
     *    /     \ /     \
     *   o---*---o---*---o
     *  0    6   3   7    1
     *
     * | CHILD 6-Node Triangle Object Node Maps:
     * |
     * | static const UInt child_0[] = { 0, 3, 5, 6, 12, 11  };
     * | static const UInt child_1[] = { 3, 1, 4, 7, 8, 13   };
     * | static const UInt child_2[] = { 5, 4, 2, 14, 9, 10  };
     * | static const UInt child_3[] = { 4, 5, 3, 14, 12, 13 };
     * |
     *
     *  Refined 6-Node Triangle Object PERMUTATION Node Maps:
     *
     *  Rotation  Polarity
     *     0          1       { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
     *     0          0       { 0, 2, 1, 5, 4, 3, 11, 10, 9, 8, 7, 6, 12, 14, 13 };
     *     1          1       { 2, 0, 1, 5, 3, 4, 10, 11, 6, 7, 8, 9, 14, 12, 13 };
     *     1          0       { 2, 1, 0, 4, 3, 5, 9, 8, 7, 6, 11, 10, 14, 13, 12 };
     *     2          1       { 1, 2, 0, 4, 5, 3, 8, 9, 10, 11, 6, 7, 13, 14, 12 };
     *     2          0       { 1, 0, 2, 3, 5, 4, 7, 6, 11, 10, 9, 8  13, 12, 14 };
     *
     **/

      static bool on_parent_face_interior(unsigned childNodeIdx, const Elem::RefinementTopology& ref_topo, unsigned& i_face, unsigned& i_ord, unsigned& n_ord)
      {
        if (on_parent_edge(childNodeIdx, ref_topo))
          return false;

        static bool doRenumber = true;

        unsigned num_child_nodes = ref_topo.num_child_nodes();
        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );
        if (cell_topo.getDimension() == 2)
          {
            i_face = 0;
            n_ord = 0;
            int n_face_n = 0;
            for (unsigned i_face_n = 0; i_face_n < num_child_nodes; i_face_n++)
              {
                if (on_parent_edge(i_face_n, ref_topo))
                  continue;
                ++n_ord;
                ++n_face_n;
              }

            i_ord = 0;
            for (unsigned i_face_n = 0; i_face_n < num_child_nodes; i_face_n++)
              {
                if (on_parent_edge(i_face_n, ref_topo))
                  continue;
                if (i_face_n == childNodeIdx)
                  {
                    if (fromTopoKey == topo_key_quad9)
                      {
                        if (doRenumber)
                          {
                            i_ord = renumber_quad_face_interior_nodes(i_face_n);
                          }
                      }
                    return true;
                  }
                ++i_ord;
              }
            return false;
          }

        unsigned n_faces = cell_topo.getFaceCount();

        for ( i_face = 0; i_face < n_faces; i_face++)
          {
            const UInt *face_nodes = ref_topo.face_node(i_face);

            n_ord = 0;
            int n_face_n = 0;
            for (unsigned i_face_n = 0; face_nodes[i_face_n] != END_UINT_ARRAY; i_face_n++)
              {
                if (on_parent_edge(face_nodes[i_face_n], ref_topo))
                  continue;
                if (face_nodes[i_face_n] < num_child_nodes)
                  ++n_ord;
                ++n_face_n;
              }

            i_ord = 0;
            for (unsigned i_face_n = 0; face_nodes[i_face_n] != END_UINT_ARRAY; i_face_n++)
              {
                if (on_parent_edge(face_nodes[i_face_n], ref_topo))
                  continue;
                unsigned j_e_node = face_nodes[i_face_n];
                if (childNodeIdx == j_e_node)
                  {
                    if (fromTopoKey == topo_key_hex27)
                      {
                        if (doRenumber)
                          {
                            i_ord = renumber_quad_face_interior_nodes(i_face_n);
                          }
                      }
                    return true;
                  }
                if (j_e_node < num_child_nodes)
                  ++i_ord;
              }
          }

        return false;
      }

      static bool on_parent_volume_interior(unsigned childNodeIdx, const Elem::RefinementTopology& ref_topo, unsigned& i_volume, unsigned& i_ord, unsigned& n_ord)
      {
        if (on_parent_face(childNodeIdx, ref_topo))
          return false;

        unsigned num_child_nodes = ref_topo.num_child_nodes();
        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );

        i_volume = 0;

        n_ord = 0;
        for (unsigned i_volume_n = 0; i_volume_n < num_child_nodes; i_volume_n++)
          {
            if (on_parent_face(i_volume_n, ref_topo))
              continue;

            ++n_ord;
          }

        i_ord = 0;
        for (unsigned i_volume_n = 0; i_volume_n < num_child_nodes; i_volume_n++)
          {
            if (on_parent_face(i_volume_n, ref_topo))
              continue;

            if (childNodeIdx == i_volume_n)
              {
                return true;
              }
            ++i_ord;
          }

        return false;
      }


      /// utility to help convert Sierra tables - this method takes the index of the child node and finds it and adds
      ///   the associated info to the ref_topo_x tables containing the new/additional refinement table information
      static void findRefinedCellTopoInfo(unsigned childNodeIdx,
                                          const Elem::RefinementTopology& ref_topo,
                                          RefTopoX_arr ref_topo_x,  // assumed good for the vertices
                                          unsigned& rank_of_subcell,
                                          unsigned& ordinal_of_subcell,
                                          unsigned& ordinal_of_node_on_subcell,
                                          unsigned& num_node_on_subcell)
      {
        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );

        //bool found = false;
        bool on_parent_edge = false;
        bool on_parent_face = false;
        bool on_parent_volume = false;
        if (on_parent_vertex(childNodeIdx))
          {
            rank_of_subcell = 0;
            ordinal_of_subcell = childNodeIdx;
            ordinal_of_node_on_subcell = 0;
            num_node_on_subcell = 1;
            return;
          }

        if ( (on_parent_edge = on_parent_edge_interior(childNodeIdx, ref_topo, ordinal_of_subcell, ordinal_of_node_on_subcell, num_node_on_subcell)))
          {
            rank_of_subcell = 1;
            return;
          }

        if ( (on_parent_face = on_parent_face_interior(childNodeIdx, ref_topo, ordinal_of_subcell, ordinal_of_node_on_subcell, num_node_on_subcell)))
          {
            rank_of_subcell = 2;
            return;
          }

        // FIXME
        if (cell_topo.getDimension() == 2)
          {
          }

        if ( (on_parent_volume = on_parent_volume_interior(childNodeIdx, ref_topo, ordinal_of_subcell, ordinal_of_node_on_subcell, num_node_on_subcell)))
          {
            rank_of_subcell = 3;
            return;
          }
        throw std::logic_error("findRefinedCellTopoInfo:: hmmmm");
      }

      static void findRefinedCellParamCoords(const Elem::RefinementTopology& ref_topo,
                                             RefTopoX_arr ref_topo_x)
      {
        shards::CellTopology parent_cell_topo ( shards::getCellTopologyData< FromTopology >() );

        unsigned num_child = ref_topo.num_child();

        for (unsigned i_child = 0; i_child < num_child; i_child++)
          {
            shards::CellTopology cell_topo = ref_topo.child_cell_topology(i_child);

            const unsigned *child_nodes = ref_topo.child_node(i_child);

            unsigned n_edges = cell_topo.getEdgeCount();
            if (n_edges == 0) n_edges = 1; // 1D edge has one "edge"
            unsigned n_faces = cell_topo.getFaceCount();
            if (parent_cell_topo.getDimension() > 1 && n_faces == 0) n_faces = 1; // 2D face has one "face"

            for (unsigned i_edge = 0; i_edge < n_edges; i_edge++)
              {
                // FIXME for 2d
                shards::CellTopology edge_topo = parent_cell_topo.getDimension()==1? parent_cell_topo : shards::CellTopology(cell_topo.getTopology( 1, i_edge));

                if (edge_topo.getNodeCount() == 3)
                  {
                    unsigned i0 = 0;
                    unsigned i1 = 0;
                    unsigned i2 = 0;

                    if (parent_cell_topo.getDimension() == 1)
                      {
                        i0 = child_nodes[0];
                        i1 = child_nodes[1];
                        i2 = child_nodes[2];
                      }
                    else
                      {
                        i0 = child_nodes[cell_topo.getTopology()->edge[i_edge].node[0]];
                        i1 = child_nodes[cell_topo.getTopology()->edge[i_edge].node[1]];
                        i2 = child_nodes[cell_topo.getTopology()->edge[i_edge].node[2]];
                      }

                    double *param_coord = ref_topo_x[i2].parametric_coordinates;
                    param_coord[0] = (ref_topo_x[i0].parametric_coordinates[0]+ref_topo_x[i1].parametric_coordinates[0])/2.;
                    param_coord[1] = (ref_topo_x[i0].parametric_coordinates[1]+ref_topo_x[i1].parametric_coordinates[1])/2.;
                    param_coord[2] = (ref_topo_x[i0].parametric_coordinates[2]+ref_topo_x[i1].parametric_coordinates[2])/2.;

                    if (0)
                      std::cout<<"param_coord in findRefinedCellParamCoords edge= " << i2 << " "
                               << param_coord[0] << " "
                               << param_coord[1] << " "
                               << param_coord[2] << std::endl;
                  }
              }

            for (unsigned i_face = 0; i_face < n_faces; i_face++)
              {
                // FIXME for 2d
                shards::CellTopology face_topo = cell_topo.getDimension()==2 ? cell_topo : shards::CellTopology(cell_topo.getTopology( 2, i_face));

                // skip triangle faces
                if (face_topo.getVertexCount() == 3)
                  continue;

                // NOTE: if this is a serendipity 8-node face, it has no interior node - only 9-noded quad faces have an interior node
                if (face_topo.getNodeCount() == 9)
                  {
                    unsigned i0 = cell_topo.getDimension()==2 ? 8 : cell_topo.getTopology()->side[i_face].node[8];
                    i0 = child_nodes[i0];

                    double *param_coord = ref_topo_x[i0].parametric_coordinates;
                    param_coord[0] = 0.0;
                    param_coord[1] = 0.0;
                    param_coord[2] = 0.0;
                    for (unsigned i_face_n=0; i_face_n < 4; i_face_n++)
                      {
                        unsigned i1 = cell_topo.getDimension()==2 ? i_face_n : cell_topo.getTopology()->side[i_face].node[i_face_n];
                        i1 = child_nodes[i1];
                        param_coord[0] += ref_topo_x[i1].parametric_coordinates[0]/4.;
                        param_coord[1] += ref_topo_x[i1].parametric_coordinates[1]/4.;
                        param_coord[2] += ref_topo_x[i1].parametric_coordinates[2]/4.;
                      }
                    if (0)
                      std::cout<<"param_coord in findRefinedCellParamCoords face= " << i0 << " "
                               << param_coord[0] << " "
                               << param_coord[1] << " "
                               << param_coord[2] << std::endl;

                  }
              }

            if (cell_topo.getDimension() == 3 && toTopoKey == topo_key_hex27)
              {
                unsigned i0 = child_nodes[centroid_node]; // Has to be a Hex27 to have an interior node
                double *param_coord = ref_topo_x[i0].parametric_coordinates;

                for (unsigned ix=0; ix < 3; ix++)
                  {
                    param_coord[ix] = 0.0;
                  }
                for (unsigned k_node = 0; k_node < 8; k_node++)
                  {
                    for (unsigned ix=0; ix < 3; ix++)
                      {
                        param_coord[ix] += ref_topo_x[k_node].parametric_coordinates[ix]/8.0;
                      }
                  }
                if (0)
                  std::cout<<"param_coord in findRefinedCellParamCoords vol= "
                           << param_coord[0] << " "
                           << param_coord[1] << " "
                           << param_coord[2] << std::endl;

              }
          }
      }


      /// continuing in the convert tables theme, this helps to find the new nodes' parametric coordinates
      static void findRefinedCellParamCoordsLinear(const Elem::RefinementTopology& ref_topo,
                                                   RefTopoX_arr ref_topo_x  // assumed good for the vertices
                                                   )
      {
        double param_coord[3];
        shards::CellTopology cell_topo ( shards::getCellTopologyData< FromTopology >() );

        unsigned num_child_nodes = ref_topo.num_child_nodes();
        for (unsigned childNodeIdx = 0; childNodeIdx < num_child_nodes; childNodeIdx++)
          {
            bool found = false;
            bool on_edge = false;
            bool on_vertex = false;
            if (childNodeIdx < FromTopology::vertex_count)
              {
                param_coord[0] = ref_topo_x[childNodeIdx].parametric_coordinates[0];
                param_coord[1] = ref_topo_x[childNodeIdx].parametric_coordinates[1];
                param_coord[2] = ref_topo_x[childNodeIdx].parametric_coordinates[2];
                found = true;
                on_vertex = true;
              }

            if (!on_vertex)
              {
                unsigned n_edges = cell_topo.getEdgeCount();
                if (n_edges == 0) n_edges = 1; // 1D face has one "edge"
                unsigned n_faces = cell_topo.getFaceCount();
                if (cell_topo.getDimension() > 1 && n_faces == 0) n_faces = 1; // 2D face has one "face"
                //unsigned n_sides = cell_topo.getSideCount();

                for (unsigned i_edge = 0; i_edge < n_edges; i_edge++)
                  {
                    const UInt *edge_nodes = ref_topo.edge_node(i_edge);

                    if (childNodeIdx == edge_nodes[2])  // FIXME
                      {
                        on_edge = true;
                        found = true;
                        param_coord[0] = (ref_topo_x[edge_nodes[0]].parametric_coordinates[0]+ref_topo_x[edge_nodes[1]].parametric_coordinates[0])/2.;
                        param_coord[1] = (ref_topo_x[edge_nodes[0]].parametric_coordinates[1]+ref_topo_x[edge_nodes[1]].parametric_coordinates[1])/2.;
                        param_coord[2] = (ref_topo_x[edge_nodes[0]].parametric_coordinates[2]+ref_topo_x[edge_nodes[1]].parametric_coordinates[2])/2.;
                        break;
                      }
                  }

                if (!found)
                  {
                    for (unsigned i_face = 0; i_face < n_faces; i_face++)
                      {

                        // FIXME for 2d
                        shards::CellTopology face_topo = cell_topo.getDimension()==2 ? cell_topo : shards::CellTopology(cell_topo.getTopology( 2, i_face));

                        // skip triangle faces
                        if (face_topo.getVertexCount() == 3)
                          continue;

                        const UInt *face_nodes = ref_topo.face_node(i_face);

                        for (unsigned j_node = 0; j_node < 9; j_node++) // FIXME
                          {
                            unsigned fn =  cell_topo.getDimension()==2 ? j_node : face_nodes[j_node];

                            if (childNodeIdx == fn)
                              {
                                found = true;

                                for (unsigned ix=0; ix < 3; ix++)
                                  {
                                    param_coord[ix] = 0.0;
                                  }
                                for (unsigned k_node = 0; k_node < 4; k_node++)
                                  {
                                    unsigned fnk = cell_topo.getDimension()==2 ? k_node : face_nodes[k_node];
                                    for (unsigned ix=0; ix < 3; ix++)
                                      {
                                        param_coord[ix] += ref_topo_x[fnk].parametric_coordinates[ix]/4.0;
                                      }
                                  }

                                break;
                              }
                          }
                      }
                  }
              }

            if (!found)
              {
                found = true;

                for (unsigned ix=0; ix < 3; ix++)
                  {
                    param_coord[ix] = 0.0;
                  }
                for (unsigned k_node = 0; k_node < 8; k_node++)
                  {
                    for (unsigned ix=0; ix < 3; ix++)
                      {
                        param_coord[ix] += ref_topo_x[k_node].parametric_coordinates[ix]/8.0;
                      }
                  }
              }

            for (unsigned ix=0; ix < 3; ix++)
              {
                ref_topo_x[childNodeIdx].parametric_coordinates[ix] = param_coord[ix];
              }
            if (0)
              std::cout<<"tmp param_coord in findRefinedCellParamCoordsLinear= " << childNodeIdx << " "
                       << param_coord[0] << " "
                       << param_coord[1] << " "
                       << param_coord[2] << std::endl;
          }

      }

    public:

      /// this is called one time (during code development) to generate and print a table of the extra refinement info

      static void
      printRefinementTopoX_Table(std::ostream& out = std::cout )
      {
        const CellTopologyData * const cell_topo_data = shards::getCellTopologyData< ToTopology >();

        CellTopology cell_topo(cell_topo_data);

        unsigned n_edges = cell_topo_data->edge_count;
        unsigned n_faces = cell_topo.getFaceCount();
        if (n_faces == 0) n_faces = 1; // 2D face has one "face"
        unsigned n_sides = cell_topo.getSideCount();
        if (0)  std::cout << "tmp  n_edges= " << n_edges << " n_faces= " << n_faces << " n_sides= " << n_sides << std::endl;

        Elem::CellTopology elem_celltopo = Elem::getCellTopology< FromTopology >();
        const Elem::RefinementTopology* ref_topo_p = Elem::getRefinementTopology(elem_celltopo);
        const Elem::RefinementTopology& ref_topo = *ref_topo_p;

        unsigned num_child = ref_topo.num_child();
        unsigned num_child_nodes = ref_topo.num_child_nodes();

        if (0) std::cout << "tmp num_child_nodes= " << num_child_nodes << " num_child= " << num_child << std::endl;

        typedef Elem::StdMeshObjTopologies::RefTopoX RefTopoX;

        RefTopoX_arr ref_topo_x = new Elem::StdMeshObjTopologies::RefinementTopologyExtraEntry[num_child_nodes];

        std::string ct_name = cell_topo.getName();
        Util::replace(ct_name, "_", "<");

        bool useIntrepid = true;
        if (useIntrepid)
          {
            for (unsigned iNode = 0; iNode < FromTopology::node_count; iNode++)
              {
                const double * param_coord = Intrepid::CellTools<double>::getReferenceNode(cell_topo, iNode);
                ref_topo_x[iNode].parametric_coordinates[0] = param_coord[0];
                ref_topo_x[iNode].parametric_coordinates[1] = param_coord[1];
                ref_topo_x[iNode].parametric_coordinates[2] = param_coord[2];
                if (0) std::cout<<"tmp param_coord= "
                                << param_coord[0] << " "
                                << param_coord[1] << " "
                                << param_coord[2] << std::endl;
              }
          }

        findRefinedCellParamCoordsLinear(ref_topo, ref_topo_x);
        findRefinedCellParamCoords(ref_topo, ref_topo_x);

        out << "\n template<> RefTopoX RefinementTopologyExtra< shards:: " << ct_name << ">  > :: refinement_topology = {" << std::endl;
        for (unsigned childNodeIdx = 0; childNodeIdx < num_child_nodes; childNodeIdx++)
          {
            //*  {Ord, Rnk-assoc, Ord-rnk-assoc, Ord-node-on-subcell, num-rnk-assoc, param-coord}

            unsigned rank_of_subcell            = 0;
            unsigned ordinal_of_subcell         = 0;
            unsigned ordinal_of_node_on_subcell = 0;
            unsigned num_node_on_subcell        = 0;

            findRefinedCellTopoInfo(childNodeIdx, ref_topo, &ref_topo_x[0], rank_of_subcell,
                                    ordinal_of_subcell, ordinal_of_node_on_subcell, num_node_on_subcell);

            double *param_coord = ref_topo_x[childNodeIdx].parametric_coordinates;
            out << "{\t" << childNodeIdx << ",\t" << rank_of_subcell << ",\t" << ordinal_of_subcell << ",\t"
                << ordinal_of_node_on_subcell << ",\t" << num_node_on_subcell
                << ",\t{" << param_coord[0] << ",\t" << param_coord[1] << ",\t" << param_coord[2] << "} }"
                << (childNodeIdx==(num_child_nodes-1)? " " : ",") << std::endl;
          }
        out << "\n};" << std::endl;
        delete[] ref_topo_x;
      }

    private:

      void fixSubsets(percept::PerceptMesh& eMesh, bool sameTopology)
      {
        if (sameTopology)
          return;

        for (unsigned i_fromPart = 0; i_fromPart < m_fromParts.size(); i_fromPart++)
          {
            mesh::Part& fromPart = *m_fromParts[i_fromPart];

            switch( fromPart.primary_entity_rank() ) {

              //case mesh::Element:
            case mesh::Edge:
            case mesh::Face:
              {
                mesh::Part& toPart = *m_toParts[i_fromPart];

                const mesh::PartVector from_subsets = fromPart.subsets();
                for (unsigned i_from_subset = 0; i_from_subset < from_subsets.size(); i_from_subset++)
                  {
                    mesh::Part& from_subset = *from_subsets[i_from_subset];
                    std::string to_subset_name = from_subset.name() + m_appendConvertString;

                    mesh::Part* to_subset_p = eMesh.getMetaData()->get_part(to_subset_name);
                    if (!to_subset_p) throw std::runtime_error("fixSubsets couldn't find part error");
                    mesh::Part& to_subset = *to_subset_p;

                    eMesh.getMetaData()->declare_part_subset(toPart, to_subset);

                    //std::cout << "fixSubsets:: declare_part_subset toPart = " << toPart.name()  << " to_subset= " <<  to_subset.name() << std::endl;
                  }
              }
              break;
            }
          }
      }

    public:
      virtual ~URP() {}


      void setNeededParts(percept::PerceptMesh& eMesh, BlockNamesType block_names_ranks, bool sameTopology=true)
      {
        EXCEPTWATCH;
        if (block_names_ranks.size() == 0)
          {
            block_names_ranks.resize(mesh::EntityRankEnd);
          }

        m_fromParts.resize(0);
        m_toParts.resize(0);


        for (unsigned irank = 0; irank < mesh::EntityRankEnd; irank++)
          {
            if (m_primaryEntityRank != irank)
              continue;

            std::vector<std::string>& block_names = block_names_ranks[irank];
            //if (block_names.size() == 0 || m_primaryEntityRank != irank)

            //const mesh::PartVector all_parts = eMesh.getMetaData()->get_parts();
            mesh::PartVector all_parts = eMesh.getMetaData()->get_parts();
            for (unsigned ib = 0; ib < block_names.size(); ib++)
              {
                bool foundPart = false;
                //for (mesh::PartVector::const_iterator i_part = all_parts.begin(); i_part != all_parts.end(); ++i_part)
                for (mesh::PartVector::iterator i_part = all_parts.begin(); i_part != all_parts.end(); ++i_part)
                  {
                    //mesh::Part * const part = *i_part ;
                    mesh::Part * part = *i_part ;

                    if (part->name() == block_names[ib])
                      {
                        foundPart = true;
                        break;
                      }
                  }
                if (!foundPart)
                  {
                    std::string msg = "UniformRefinerPattern::setNeededParts unknown block name: " + block_names[ib];
                    throw std::runtime_error(msg.c_str());
                  }
              }

            for (mesh::PartVector::iterator i_part = all_parts.begin(); i_part != all_parts.end(); ++i_part)
              {
                mesh::Part *  part = *i_part ;

                // FIXME - is there a better way to determine if a part is one of the "standard" parts?
                if (part->name()[0] == '{')
                  continue;

                bool doThisPart = (block_names.size() == 0);
                for (unsigned ib = 0; ib < block_names.size(); ib++)
                  {
                    if (part->name() == block_names[ib])
                      {
                        doThisPart=true;
                        break;
                      }
                  }
                doThisPart = doThisPart && ( part->primary_entity_rank() == m_primaryEntityRank );

                //                 std::cout << "setNeededParts:: part name= " << part->name() << " doThisPart= " << doThisPart
                //                           << "  part->primary_entity_rank() = " <<  part->primary_entity_rank() << std::endl;

                if (doThisPart)
                  {
                    switch( part->primary_entity_rank() ) {

                    case mesh::Edge:
                    case mesh::Element:
                    case mesh::Face:
                      {
                        mesh::Part *  block_to=0;
                        //mesh::Part* block_to = 0;
                        if (sameTopology)
                          {
                            block_to = part;
                          }
                        else
                          {
                            block_to = &eMesh.getMetaData()->declare_part(part->name() + m_appendConvertString, part->primary_entity_rank());
                            mesh::set_cell_topology< ToTopology  >( *block_to );
                            stk::io::put_io_part_attribute(*block_to);
                          }

                        //std::cout << "setNeededParts:: declare_part name= " << (part->name() + m_appendConvertString) << std::endl;

                        m_fromParts.push_back(part);
                        m_toParts.push_back(block_to);
                      }
                      break;
                    }
                  }
              }
          }
        fixSubsets(eMesh, sameTopology);

        eMesh.getMetaData()->declare_part(m_oldElementsPartName+toString(m_primaryEntityRank), m_primaryEntityRank);

#if 0
        if (m_primaryEntityRank == mesh::Element)
          {
            //mesh::Part& oldElementsPart =
            eMesh.getMetaData()->declare_part(m_oldElementsPartName+toString(mesh::Element), mesh::Element);
            eMesh.getMetaData()->declare_part(m_oldElementsPartName+toString(mesh::Edge), mesh::Edge);
            //mesh::Part& oldElementsPart = eMesh.getMetaData()->declare_part(m_oldElementsPartName);
            //mesh::set_cell_topology< FromTopology  >( oldElementsPart );
            //stk::io::put_io_part_attribute( oldElementsPart );
          }
#endif
      }

      void change_entity_parts(percept::PerceptMesh& eMesh, Entity& old_owning_elem, Entity& newElement)
      {
        std::vector<stk::mesh::Part*> add_parts(1);
        std::vector<stk::mesh::Part*> remove_parts;

        bool found = false;
        for (unsigned i_part = 0; i_part < m_fromParts.size(); i_part++)
          {
            if (old_owning_elem.bucket().member(*m_fromParts[i_part]))
              {
                add_parts[0] = m_toParts[i_part];
                if (0)
                  {
                    std::cout << "tmp changing newElement " << newElement.identifier()
                              << " rank= " << newElement.entity_rank()
                              << " from part= " << m_fromParts[i_part]->name()
                              << " to part= " << m_toParts[i_part]->name()
                              << " for old elem= " << old_owning_elem.identifier()
                              << " rank= " << old_owning_elem.entity_rank()
                              << std::endl;
                  }
                eMesh.getBulkData()->change_entity_parts( newElement, add_parts, remove_parts );
                //return;
                found = true;
              }
          }
        if (!found)
          {
            std::cout << "m_fromParts= " << m_fromParts << std::endl;
            for (unsigned i_part = 0; i_part < m_fromParts.size(); i_part++)
              {
                std::cout << "i_part = " << i_part << " m_fromParts= " << m_fromParts[i_part]->name() << std::endl;
              }
            throw std::runtime_error("URP::change_entity_parts couldn't find part");
          }
      }
    };

    template<typename FromTopology,  typename ToTopology, int NumToCreate, class OptionalTag=void>
    class UniformRefinerPattern : public URP<FromTopology, ToTopology> //, public URP1<FromTopology, ToTopology>
    {
    public:
    };


  }
}



// all the patterns

// homogeneous refine
#include "UniformRefinerPattern_Quad4_Quad4_4.hpp"
#include "UniformRefinerPattern_Line2_Line2_2_sierra.hpp"
#include "UniformRefinerPattern_ShellLine2_ShellLine2_2_sierra.hpp"
#include "UniformRefinerPattern_Quad4_Quad4_4_sierra.hpp"
#include "UniformRefinerPattern_Tri3_Tri3_4_sierra.hpp"
#include "UniformRefinerPattern_ShellTri3_ShellTri3_4_sierra.hpp"
#include "UniformRefinerPattern_ShellQuad4_ShellQuad4_4_sierra.hpp"
#include "UniformRefinerPattern_Tet4_Tet4_8_sierra.hpp"
#include "UniformRefinerPattern_Hex8_Hex8_8_sierra.hpp"
#include "UniformRefinerPattern_Wedge6_Wedge6_8_sierra.hpp"

#include "UniformRefinerPattern_Line3_Line3_2_sierra.hpp"
#include "UniformRefinerPattern_Tri6_Tri6_4_sierra.hpp"
#include "UniformRefinerPattern_Quad9_Quad9_4_sierra.hpp"
#include "UniformRefinerPattern_Hex27_Hex27_8_sierra.hpp"
#include "UniformRefinerPattern_Tet10_Tet10_8_sierra.hpp"


// enrich
// line2-line3
#include "UniformRefinerPattern_Quad4_Quad9_1_sierra.hpp"
#include "UniformRefinerPattern_Quad4_Quad8_1_sierra.hpp"
#include "UniformRefinerPattern_Tri3_Tri6_1_sierra.hpp"
#include "UniformRefinerPattern_Tet4_Tet10_1_sierra.hpp"
#include "UniformRefinerPattern_Hex8_Hex27_1_sierra.hpp"
#include "UniformRefinerPattern_Hex8_Hex20_1_sierra.hpp"
#include "UniformRefinerPattern_Wedge6_Wedge15_1_sierra.hpp"

// convert topology
#include "UniformRefinerPattern_Quad4_Tri3_6.hpp"
#include "UniformRefinerPattern_Quad4_Tri3_4.hpp"
#include "UniformRefinerPattern_Quad4_Tri3_2.hpp"
#include "UniformRefinerPattern_Hex8_Tet4_24.hpp"
#include "UniformRefinerPattern_Hex8_Tet4_6_12.hpp"


namespace stk {
  namespace adapt {

    // refine
    typedef  UniformRefinerPattern<shards::Line<2>,          shards::Line<2>,          2, SierraPort >            Line2_Line2_2;
    typedef  UniformRefinerPattern<shards::ShellLine<2>,     shards::ShellLine<2>,     2, SierraPort >            ShellLine2_ShellLine2_2;
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4 >                        Quad4_Quad4_4_Old;
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort >            Quad4_Quad4_4;

    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort >            Quad4_Quad4_4_Sierra;
    typedef  UniformRefinerPattern<shards::Triangle<3>,      shards::Triangle<3>,      4, SierraPort >            Tri3_Tri3_4;
    typedef  UniformRefinerPattern<shards::ShellTriangle<3>, shards::ShellTriangle<3>, 4, SierraPort >            ShellTri3_ShellTri3_4;
    typedef  UniformRefinerPattern<shards::ShellQuadrilateral<4>, shards::ShellQuadrilateral<4>, 4, SierraPort >  ShellQuad4_ShellQuad4_4;
    typedef  UniformRefinerPattern<shards::Tetrahedron<4>,   shards::Tetrahedron<4>,   8, SierraPort >            Tet4_Tet4_8;
    typedef  UniformRefinerPattern<shards::Hexahedron<8>,    shards::Hexahedron<8>,    8, SierraPort >            Hex8_Hex8_8;
    typedef  UniformRefinerPattern<shards::Wedge<6>,         shards::Wedge<6>,         8, SierraPort >            Wedge6_Wedge6_8;

    typedef  UniformRefinerPattern<shards::Line<3>,          shards::Line<3>,          2 >                        Line3_Line3_2;
    typedef  UniformRefinerPattern<shards::Triangle<6>,      shards::Triangle<6>,      4, SierraPort >            Tri6_Tri6_4;
    typedef  UniformRefinerPattern<shards::Quadrilateral<9>, shards::Quadrilateral<9>, 4, SierraPort >            Quad9_Quad9_4;
    typedef  UniformRefinerPattern<shards::Hexahedron<27>,   shards::Hexahedron<27>,   8, SierraPort >            Hex27_Hex27_8;
    typedef  UniformRefinerPattern<shards::Tetrahedron<10>,  shards::Tetrahedron<10>,   8, SierraPort >           Tet10_Tet10_8;

    // enrich
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<9>, 1, SierraPort >            Quad4_Quad9_1;
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<8>, 1, SierraPort >            Quad4_Quad8_1;
    typedef  UniformRefinerPattern<shards::Triangle<3>,      shards::Triangle<6>,      1, SierraPort >            Tri3_Tri6_1;
    typedef  UniformRefinerPattern<shards::Tetrahedron<4>,   shards::Tetrahedron<10>,  1, SierraPort >            Tet4_Tet10_1;
    typedef  UniformRefinerPattern<shards::Hexahedron<8>,    shards::Hexahedron<27>,   1, SierraPort >            Hex8_Hex27_1;
    typedef  UniformRefinerPattern<shards::Hexahedron<8>,    shards::Hexahedron<20>,   1, SierraPort >            Hex8_Hex20_1;
    typedef  UniformRefinerPattern<shards::Wedge<6>,         shards::Wedge<15>,        1, SierraPort >            Wedge6_Wedge15_1;

    // convert
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>,      2 >                        Quad4_Tri3_2;
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>,      4, Specialization >        Quad4_Tri3_4;
    typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>,      6 >                        Quad4_Tri3_6;
    typedef  UniformRefinerPattern<shards::Hexahedron<8>,    shards::Tetrahedron<4>,  24 >                        Hex8_Tet4_24;
    typedef  UniformRefinerPattern<shards::Hexahedron<8>,    shards::Tetrahedron<4>,   6 >                        Hex8_Tet4_6_12;

  }
}



#endif
