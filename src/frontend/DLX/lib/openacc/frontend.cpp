/*!
 * 
 * \file lib/openacc/frontend.cpp
 *
 * \author Tristan Vanderbruggen
 *
 */

#include "DLX/Core/frontend.hpp"
#include "DLX/Core/directives.hpp"
#include "DLX/Core/constructs.hpp"
#include "DLX/Core/clauses.hpp"
#include "DLX/OpenACC/language.hpp"

#include <iostream>

#include <cassert>

#include "rose.h"

#ifndef OPENACC_MULTIDEV
# define OPENACC_MULTIDEV 1
#endif
#ifdef OPENACC_DATA_ACCESS
# define OPENACC_DATA_ACCESS 1
#endif

namespace DLX {

namespace Frontend {

/*!
 * \addtogroup grp_dlx_openacc_frontend
 * @{
 */

template <>
template <>
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_data>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_data> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  SgPragmaDeclaration * pragma_decl = isSgPragmaDeclaration(directive_node);
  assert(pragma_decl != NULL);

  construct->assoc_nodes.parent_scope = isSgScopeStatement(pragma_decl->get_parent());
  assert(construct->assoc_nodes.parent_scope != NULL);
  construct->assoc_nodes.data_scope = isSgScopeStatement(SageInterface::getNextStatement(pragma_decl));
  assert(construct->assoc_nodes.data_scope != NULL);

  return true;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_parallel>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_parallel> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  SgPragmaDeclaration * pragma_decl = isSgPragmaDeclaration(directive_node);
  assert(pragma_decl != NULL);

  construct->assoc_nodes.parent_scope = isSgScopeStatement(pragma_decl->get_parent());
  assert(construct->assoc_nodes.parent_scope != NULL);

  SgStatement * stmt = SageInterface::getNextStatement(pragma_decl);
  construct->assoc_nodes.parallel_scope = isSgScopeStatement(stmt);
  if (construct->assoc_nodes.parallel_scope != NULL)
    construct->assoc_nodes.scoped_directive = NULL;
  else {
    std::map<SgLocatedNode *, directive_t *>::const_iterator it = translation_map.find(stmt);
    assert(it != translation_map.end());
    construct->assoc_nodes.scoped_directive = it->second;
  }

  return true;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_kernel>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_kernel> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  SgPragmaDeclaration * pragma_decl = isSgPragmaDeclaration(directive_node);
  assert(pragma_decl != NULL);

  construct->assoc_nodes.parent_scope = isSgScopeStatement(pragma_decl->get_parent());
  assert(construct->assoc_nodes.parent_scope != NULL);
  construct->assoc_nodes.kernel_scope = isSgScopeStatement(SageInterface::getNextStatement(pragma_decl));
  assert(construct->assoc_nodes.kernel_scope != NULL);

  return true;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_loop>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_loop> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  SgPragmaDeclaration * pragma_decl = isSgPragmaDeclaration(directive_node);
  assert(pragma_decl != NULL);

  construct->assoc_nodes.parent_scope = isSgScopeStatement(pragma_decl->get_parent());
  assert(construct->assoc_nodes.parent_scope != NULL);
  construct->assoc_nodes.for_loop = isSgForStatement(SageInterface::getNextStatement(pragma_decl));
  assert(construct->assoc_nodes.for_loop != NULL);

  return true;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_host_data>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_host_data> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  return false;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_declare>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_declare> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  return false;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_cache>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_cache> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  return false;
}

template <> 
template <> 
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_update>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_update> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::findAssociatedNodes<OpenACC::language_t::e_acc_construct_blank>(
  SgLocatedNode * directive_node,
  Directives::construct_t<OpenACC::language_t, OpenACC::language_t::e_acc_construct_blank> * construct,
  const std::map<SgLocatedNode *, directive_t *> & translation_map
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_if>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_if> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse<SgExpression *>(clause->parameters.condition);
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_async>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_async> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse<SgExpression *>(clause->parameters.sync_tag);
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_num_gangs>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_num_gangs> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_singleton<SgExpression *>(clause->parameters.exp, '(', ')');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_num_workers>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_num_workers> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_vector_length>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_vector_length> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_reduction>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_reduction> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_copy>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_copy> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_copyin>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_copyin> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_copyout>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_copyout> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_create>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_create> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_present>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_present> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_present_or_copy>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_present_or_copy> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_present_or_copyin>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_present_or_copyin> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_present_or_copyout>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_present_or_copyout> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_present_or_create>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_present_or_create> * clause
) {
  DLX::Frontend::Parser parser(directive_str, directive_node);
  bool res = parser.parse_list(clause->parameters.data_sections, '(', ')', ',');
  if (res) directive_str = parser.getDirectiveString();
  return res;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_deviceptr>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_deviceptr> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_private>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_private> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_firstprivate>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_firstprivate> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_use_device>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_use_device> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_device_resident>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_device_resident> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_collapse>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_collapse> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_auto>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_auto> * clause
) {
  assert(directive_str.size() == 0 || directive_str[0] != '(');
  return true;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_gang>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_gang> * clause
) {
  assert(directive_str.size() == 0 || directive_str[0] != '(');

  clause->parameters.dimension_id = -1;
  if (directive_str[0] == '[') {
    /// \todo read number in dimension_id, find matching ']'
  }
  else clause->parameters.dimension_id = 0;

  assert(clause->parameters.dimension_id != -1);

  return true;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_worker>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_worker> * clause
) {
  assert(directive_str.size() == 0 || directive_str[0] != '(');

  clause->parameters.dimension_id = -1;
  if (directive_str[0] == '[') {
    /// \todo read number in dimension_id, find matching ']'
  }
  else clause->parameters.dimension_id = 0;

  assert(clause->parameters.dimension_id != -1);

  return true;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_vector>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_vector> * clause
) {
  assert(directive_str.size() == 0 || directive_str[0] != '(');
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_seq>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_seq> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_independent>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_independent> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_host>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_host> * clause
) {
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_device>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_device> * clause
) {
  return false;
}

#if OPENACC_MULTIDEV

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_split>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_split> * clause
) {
  /// \todo
  assert(false);
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_devices>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_devices> * clause
) {
  /// \todo
  assert(false);
  return false;
}

#endif

#if OPENACC_DATA_ACCESS

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_read>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_read> * clause
) {
  /// \todo
  assert(false);
  return false;
}

template <>
template <>
bool Frontend<OpenACC::language_t>::parseClauseParameters<OpenACC::language_t::e_acc_clause_write>(
  std::string & directive_str,
  SgLocatedNode * directive_node,
  Directives::clause_t<OpenACC::language_t, OpenACC::language_t::e_acc_clause_write> * clause
) {
  /// \todo
  assert(false);
  return false;
}

#endif

template <>
bool Frontend<OpenACC::language_t>::build_graph() {
  /// \todo Frontend<OpenACC::language_t>::build_graph()
  return false;
}

/** @} */

}

}

