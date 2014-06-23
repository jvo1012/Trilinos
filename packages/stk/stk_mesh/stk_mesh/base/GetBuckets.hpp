/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_GetBucket_hpp
#define stk_mesh_GetBucket_hpp

#include <stk_mesh/base/Types.hpp>      // for PartVector
namespace stk { namespace mesh { class Bucket; } }

namespace stk {
namespace mesh {

/** \addtogroup stk_mesh_module
 *  \{
 */

/* \brief  Get the parts from the union part vector that the bucket is
 *         contained in.
 */
void get_involved_parts( const PartVector & union_parts,
                         const Bucket & candidate,
                         PartVector & involved_parts);

/** \} */

} // namespace mesh
} // namespace stk

#endif

