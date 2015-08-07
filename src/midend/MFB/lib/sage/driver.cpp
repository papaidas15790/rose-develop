
#include "sage3basic.h"

#include "MFB/Sage/driver.hpp"
#include "MFB/Sage/api.hpp"

#include <boost/filesystem.hpp>

#ifndef PATCHING_SAGE_BUILDER_ISSUES
#  define PATCHING_SAGE_BUILDER_ISSUES 1
#endif

namespace MFB {

/*!
 * \addtogroup grp_mfb_sage_driver
 * @{
*/

bool ignore(const std::string & name) {
  return name.find("__builtin") == 0 || name.find("__frontend_specific_variable_to_provide_header_file_path") == 0;
}

bool ignore(SgScopeStatement * scope) {
  return isSgBasicBlock(scope);
}

/*template <>
bool Driver<Sage>::resolveValidParent<SgTypedefSymbol>(SgTypedefSymbol * symbol) {
  SgSymbol * parent = NULL;
  
  if (p_valid_symbols.find(symbol) != p_valid_symbols.end()) return true;

  SgNamespaceDefinitionStatement * namespace_scope = isSgNamespaceDefinitionStatement(symbol->get_scope());
  SgClassDefinition              * class_scope     = isSgClassDefinition             (symbol->get_scope());
  if (namespace_scope != NULL) {
    SgNamespaceDeclarationStatement * parent_decl = namespace_scope->get_namespaceDeclaration();
    assert(parent_decl != NULL);
    parent = SageInterface::lookupNamespaceSymbolInParentScopes(parent_decl->get_name(), parent_decl->get_scope());
    assert(parent != NULL);

    if (!resolveValidParent<SgNamespaceSymbol>((SgNamespaceSymbol *)parent)) return false;
    assert(p_valid_symbols.find(parent) != p_valid_symbols.end());
  }
  else if (class_scope != NULL) {
    SgClassDeclaration * parent_decl = class_scope->get_declaration();
    assert(parent_decl != NULL);
    parent = SageInterface::lookupClassSymbolInParentScopes(parent_decl->get_name(), parent_decl->get_scope());
    assert(parent != NULL);

    if (!resolveValidParent<SgClassSymbol>((SgClassSymbol *)parent)) return false;
    assert(p_valid_symbols.find(parent) != p_valid_symbols.end());
  }

  p_valid_symbols.insert(symbol);
  p_parent_map.insert(std::pair<SgSymbol *, SgSymbol *>(symbol, parent));
  p_typedef_symbols.insert(symbol);

  return true;
}

template <>
void  Driver<Sage>::loadSymbols<SgTypedefDeclaration>(size_t file_id, SgSourceFile * file) {
  std::vector<SgTypedefDeclaration *> typedef_decl = SageInterface::querySubTree<SgTypedefDeclaration>(file);

  std::vector<SgTypedefDeclaration *>::const_iterator it_typedef_decl;
  for (it_typedef_decl = typedef_decl.begin(); it_typedef_decl != typedef_decl.end(); it_typedef_decl++) {
    SgTypedefDeclaration * typedef_decl = *it_typedef_decl;

    if (ignore(typedef_decl->get_scope())) continue;
    if (ignore(typedef_decl->get_name().getString())) continue;

    SgTypedefSymbol * typedef_sym = SageInterface::lookupTypedefSymbolInParentScopes(typedef_decl->get_name(), typedef_decl->get_scope());
    assert(typedef_sym != NULL);

    if (resolveValidParent<SgTypedefSymbol>(typedef_sym))
      p_symbol_to_file_id_map.insert(std::pair<SgSymbol *, size_t>(typedef_sym, file_id));
  }
}*/

template <>
void Driver<Sage>::loadSymbols<SgDeclarationStatement>(size_t file_id, SgSourceFile * file) {
  loadSymbols<SgNamespaceDeclarationStatement>(file_id, file);
  loadSymbols<SgFunctionDeclaration>(file_id, file);
  loadSymbols<SgClassDeclaration>(file_id, file);
  loadSymbols<SgVariableDeclaration>(file_id, file);
  loadSymbols<SgMemberFunctionDeclaration>(file_id, file);

//loadSymbols<SgTypedefDeclaration>(file_id, file);
}

size_t Driver<Sage>::getFileID(const boost::filesystem::path & path) const {
  std::map<boost::filesystem::path, size_t>::const_iterator it_file_id = path_to_id_map.find(path);
  assert(it_file_id != path_to_id_map.end());
  return it_file_id->second;
}

size_t Driver<Sage>::getFileID(SgSourceFile * source_file) const {
  std::map<SgSourceFile *, size_t>::const_iterator it_file_id = file_to_id_map.find(source_file);
  assert(it_file_id != file_to_id_map.end());
  return it_file_id->second;
}

size_t Driver<Sage>::getFileID(SgScopeStatement * scope) const {
  SgFile * enclosing_file = SageInterface::getEnclosingFileNode(scope);
  assert(enclosing_file != NULL); // FIXME Contingency : Scope Stack

  SgSourceFile * enclosing_source_file = isSgSourceFile(enclosing_file);
  assert(enclosing_source_file != NULL);

  return getFileID(enclosing_source_file);
}

Driver<Sage>::Driver(SgProject * project_) :
  project(project_),
  file_id_counter(1), // 0 is reserved
  path_to_id_map(),
  id_to_file_map(),
  file_to_id_map(),
  file_id_to_accessible_file_id_map(),
  p_symbol_to_file_id_map(),
  p_valid_symbols(),
  p_parent_map(),
  p_namespace_symbols(),
  p_function_symbols(),
  p_class_symbols(),
  p_variable_symbols(),
  p_member_function_symbols(),
  p_typedef_symbols(),
  p_type_scope_map()
{ 
  assert(project != NULL);

  if (!CommandlineProcessing::isCppFileNameSuffix("hpp"))
    CommandlineProcessing::extraCppSourceFileSuffixes.push_back("hpp");

  if (!CommandlineProcessing::isCppFileNameSuffix("h"))
    CommandlineProcessing::extraCppSourceFileSuffixes.push_back("h");

  // Load existing files
  const std::vector<SgFile *> & files = project->get_fileList_ptr()->get_listOfFiles();
  std::vector<SgFile *>::const_iterator it_file;
  for (it_file = files.begin(); it_file != files.end(); it_file++) {
    SgSourceFile * src_file = isSgSourceFile(*it_file);
    if (src_file != NULL) add(src_file);
  }
}

Driver<Sage>::~Driver() {}

size_t Driver<Sage>::add(SgSourceFile * file) {
  size_t id = file_id_counter++;

  std::cerr << "[Info] (MFB::Driver<Sage>::Driver<Sage>) Add file: " << file->get_sourceFileNameWithoutPath() << " as File #" << id << std::endl;

  id_to_file_map.insert(std::pair<size_t, SgSourceFile *>(id, file));
  file_to_id_map.insert(std::pair<SgSourceFile *, size_t>(file, id));
  file_id_to_accessible_file_id_map.insert(std::pair<size_t, std::set<size_t> >(id, std::set<size_t>())).first->second.insert(id);

  /// \todo detect other accessible file (opt: add recursively)

  loadSymbols<SgDeclarationStatement>(id, file);

  return id;
}

size_t Driver<Sage>::add(const boost::filesystem::path & path) {
  if (!boost::filesystem::exists(path) || !boost::filesystem::is_regular_file(path)) {
    std::cerr << "[Driver<Sage>] Cannot find: " << path << std::endl;
    assert(false);
  }

  std::map<boost::filesystem::path, size_t>::const_iterator it_file_id = path_to_id_map.find(path);
  if (it_file_id != path_to_id_map.end())
    return it_file_id->second;
  else {
    SgSourceFile * file = isSgSourceFile(SageBuilder::buildFile(path.string(), path.string(), project));
    assert(file != NULL);
    file->set_skip_unparse(true);
    file->set_skipfinalCompileStep(true);

    size_t id = add(file);

    path_to_id_map.insert(std::pair<boost::filesystem::path, size_t>(path, id));

    return id;
  }
}

size_t Driver<Sage>::create(const boost::filesystem::path & path) {
  assert(path_to_id_map.find(path) == path_to_id_map.end());

  if (boost::filesystem::exists(path))
    boost::filesystem::remove(path);

  std::map<boost::filesystem::path, size_t>::const_iterator it_file_id = path_to_id_map.find(path);
  assert(it_file_id == path_to_id_map.end());
    
  SgSourceFile * file = isSgSourceFile(SageBuilder::buildFile(path.string(), path.string(), project));
  assert(file != NULL);
  SageInterface::attachComment(file, "/* File generated by Driver<Model>::getFileID(\"" + path.string() + "\") */");
  file->set_skipfinalCompileStep(true);

  size_t id = file_id_counter++;

  id_to_file_map.insert(std::pair<size_t, SgSourceFile *>(id, file));
  file_to_id_map.insert(std::pair<SgSourceFile *, size_t>(file, id));
  path_to_id_map.insert(std::pair<boost::filesystem::path, size_t>(path, id));
  file_id_to_accessible_file_id_map.insert(std::pair<size_t, std::set<size_t> >(id, std::set<size_t>())).first->second.insert(id);

  return id;
}

void Driver<Sage>::setUnparsedFile(size_t file_id) const {
  std::map<size_t, SgSourceFile *>::const_iterator it_file = id_to_file_map.find(file_id);
  assert(it_file != id_to_file_map.end());
  it_file->second->set_skip_unparse(false);
}

void Driver<Sage>::setCompiledFile(size_t file_id) const {
  std::map<size_t, SgSourceFile *>::const_iterator it_file = id_to_file_map.find(file_id);
  assert(it_file != id_to_file_map.end());
  it_file->second->set_skipfinalCompileStep(false);
}

boost::filesystem::path resolve(
    const boost::filesystem::path & p,
    const boost::filesystem::path & base = boost::filesystem::current_path()
) {
//    boost::filesystem::path abs_p = boost::filesystem::absolute(p,base);
    boost::filesystem::path abs_p = p;
    boost::filesystem::path result;
    for(boost::filesystem::path::iterator it=abs_p.begin();
        it!=abs_p.end();
        ++it)
    {
        if(*it == "..")
        {
            // /a/b/.. is not necessarily /a if b is a symbolic link
            if(boost::filesystem::is_symlink(result) )
                result /= *it;
            // /a/b/../.. is not /a/b/.. under most circumstances
            // We can end up with ..s in our result because of symbolic links
            else if(result.filename() == "..")
                result /= *it;
            // Otherwise it should be safe to resolve the parent
            else
                result = result.parent_path();
        }
        else if(*it == ".")
        {
            // Ignore
        }
        else
        {
            // Just cat other path entries
            result /= *it;
        }
    }
    return result;
}

void Driver<Sage>::addIncludeDirectives(size_t target_file_id, size_t header_file_id) {
  std::string header_file_name;

  std::map<size_t, SgSourceFile *>::const_iterator it_file = id_to_file_map.find(target_file_id);
  assert(it_file != id_to_file_map.end());
  SgSourceFile * target_file = it_file->second;
  assert(target_file != NULL);

  it_file = id_to_file_map.find(header_file_id);
  assert(it_file != id_to_file_map.end());
  SgSourceFile * header_file = it_file->second;
  assert(header_file != NULL);

  header_file_name = header_file->getFileName(); /// \todo find minimum file name (based on include path...)
  const std::vector<std::string> & arg_list = project->get_originalCommandLineArgumentList();
  std::vector<std::string>::const_iterator it_arg;
  for (it_arg = arg_list.begin(); it_arg != arg_list.end(); it_arg++) {
    if ((*it_arg).find("-I") == 0) {
      std::string inc_path = resolve(boost::filesystem::path((*it_arg).substr(2))).string();
      if (header_file_name.find(inc_path) == 0) {
        header_file_name = header_file_name.substr(inc_path.length());
        while (header_file_name[0] == '/') header_file_name = header_file_name.substr(1);
        break;
      }
    }
  }

  SageInterface::insertHeader(target_file, header_file_name);
}

void Driver<Sage>::addExternalHeader(size_t file_id, std::string header_name, bool is_system_header) {
  std::map<size_t, SgSourceFile *>::iterator it_file = id_to_file_map.find(file_id);
  assert(it_file != id_to_file_map.end());
  
  SgSourceFile * file = it_file->second;
  assert(file != NULL);

  SageInterface::insertHeader(file, header_name, is_system_header);
}

void Driver<Sage>::addPragmaDecl(size_t file_id, std::string str) {
  std::map<size_t, SgSourceFile *>::iterator it_file = id_to_file_map.find(file_id);
  assert(it_file != id_to_file_map.end());
  
  SgSourceFile * file = it_file->second;
  assert(file != NULL);
  SgGlobal * global = file->get_globalScope();

  SageInterface::prependStatement(SageBuilder::buildPragmaDeclaration(str, global), global);
}

void Driver<Sage>::addPointerToTopParentDeclaration(SgSymbol * symbol, size_t file_id) {
  SgSymbol * parent = symbol;
  std::map<SgSymbol *, SgSymbol *>::const_iterator it_parent = p_parent_map.find(symbol);
  assert(it_parent != p_parent_map.end());
  while (it_parent->second != NULL) {
    parent = it_parent->second;
    it_parent = p_parent_map.find(parent);
    assert(it_parent != p_parent_map.end());
  }
  assert(parent != NULL);

  SgDeclarationStatement * decl_to_add = NULL;
  SgVariableSymbol * var_sym = isSgVariableSymbol(parent);
  if (var_sym != NULL) {
    assert(var_sym == symbol);

    SgInitializedName * init_name = isSgInitializedName(var_sym->get_symbol_basis());
    assert(init_name != NULL);

    assert(false); // TODO
  }
  else
    decl_to_add = isSgDeclarationStatement(parent->get_symbol_basis());
  assert(decl_to_add != NULL);

  std::map<size_t, SgSourceFile *>::iterator it_file = id_to_file_map.find(file_id);
  assert(it_file != id_to_file_map.end());
  SgSourceFile * file = it_file->second;
  assert(file != NULL);

  SgGlobal * global_scope = file->get_globalScope();
  assert(global_scope != NULL);

  const std::vector<SgDeclarationStatement *> & declaration_list = global_scope->getDeclarationList();
  if (find(declaration_list.begin(), declaration_list.end(), decl_to_add) == declaration_list.end())
    SageInterface::prependStatement(decl_to_add, global_scope);
}

api_t * Driver<Sage>::getAPI(size_t file_id) const {
  api_t * api = new api_t();

  // Namespaces are not local to a file. If a namespace have been detected in any file, it will be in the API
  std::set<SgNamespaceSymbol *>::const_iterator it_namespace_symbol;
  for (it_namespace_symbol = p_namespace_symbols.begin(); it_namespace_symbol != p_namespace_symbols.end(); it_namespace_symbol++)
    api->namespace_symbols.insert(*it_namespace_symbol);

  std::map<SgSymbol *, size_t>::const_iterator it_sym_decl_file_id;

  std::set<SgFunctionSymbol *>::const_iterator it_function_symbol;
  for (it_function_symbol = p_function_symbols.begin(); it_function_symbol != p_function_symbols.end(); it_function_symbol++) {
    it_sym_decl_file_id = p_symbol_to_file_id_map.find(*it_function_symbol);
    assert(it_sym_decl_file_id != p_symbol_to_file_id_map.end());

    if (it_sym_decl_file_id->second == file_id)
      api->function_symbols.insert(*it_function_symbol);
  }

  std::set<SgClassSymbol *>::const_iterator it_class_symbol;
  for (it_class_symbol = p_class_symbols.begin(); it_class_symbol != p_class_symbols.end(); it_class_symbol++) {
    it_sym_decl_file_id = p_symbol_to_file_id_map.find(*it_class_symbol);
//  assert(it_sym_decl_file_id != p_symbol_to_file_id_map.end());

    if (it_sym_decl_file_id != p_symbol_to_file_id_map.end() && it_sym_decl_file_id->second == file_id)
      api->class_symbols.insert(*it_class_symbol);
  }

  std::set<SgVariableSymbol *>::const_iterator it_variable_symbol;
  for (it_variable_symbol = p_variable_symbols.begin(); it_variable_symbol != p_variable_symbols.end(); it_variable_symbol++) {
    it_sym_decl_file_id = p_symbol_to_file_id_map.find(*it_variable_symbol);
    assert(it_sym_decl_file_id != p_symbol_to_file_id_map.end());

    if (it_sym_decl_file_id->second == file_id)
      api->variable_symbols.insert(*it_variable_symbol);
  }

  std::set<SgMemberFunctionSymbol *>::const_iterator it_member_function_symbol;
  for (it_member_function_symbol = p_member_function_symbols.begin(); it_member_function_symbol != p_member_function_symbols.end(); it_member_function_symbol++) {
    it_sym_decl_file_id = p_symbol_to_file_id_map.find(*it_member_function_symbol);
    assert(it_sym_decl_file_id != p_symbol_to_file_id_map.end());

    if (it_sym_decl_file_id->second == file_id)
      api->member_function_symbols.insert(*it_member_function_symbol);
  }

  return api;
}

api_t * Driver<Sage>::getAPI(const std::set<size_t> & file_ids) const {
  assert(file_ids.size() > 0);

  std::set<size_t>::const_iterator it = file_ids.begin();

  api_t * api = getAPI(*it);
  it++;

  while (it != file_ids.end()) {

    std::cerr << "Add file " << *it << " to API..." << std::endl;

    api_t * tmp = getAPI(*it);
    merge_api(api, tmp);
    delete tmp;
    it++;
  }

  return api;
}

api_t * Driver<Sage>::getAPI() const {
  api_t * api = new api_t();

  std::set<SgNamespaceSymbol *>::const_iterator it_namespace_symbol;
  for (it_namespace_symbol = p_namespace_symbols.begin(); it_namespace_symbol != p_namespace_symbols.end(); it_namespace_symbol++)
    api->namespace_symbols.insert(*it_namespace_symbol);

  std::set<SgFunctionSymbol *>::const_iterator it_function_symbol;
  for (it_function_symbol = p_function_symbols.begin(); it_function_symbol != p_function_symbols.end(); it_function_symbol++)
    api->function_symbols.insert(*it_function_symbol);

  std::set<SgClassSymbol *>::const_iterator it_class_symbol;
  for (it_class_symbol = p_class_symbols.begin(); it_class_symbol != p_class_symbols.end(); it_class_symbol++)
    api->class_symbols.insert(*it_class_symbol);

  std::set<SgVariableSymbol *>::const_iterator it_variable_symbol;
  for (it_variable_symbol = p_variable_symbols.begin(); it_variable_symbol != p_variable_symbols.end(); it_variable_symbol++)
    api->variable_symbols.insert(*it_variable_symbol);

  std::set<SgMemberFunctionSymbol *>::const_iterator it_member_function_symbol;
  for (it_member_function_symbol = p_member_function_symbols.begin(); it_member_function_symbol != p_member_function_symbols.end(); it_member_function_symbol++)
    api->member_function_symbols.insert(*it_member_function_symbol);

  return api;
}

SgGlobal * Driver<Sage>::getGlobalScope(file_id_t id) const {
  std::map<file_id_t, SgSourceFile *>::const_iterator it = id_to_file_map.find(id);
  return it != id_to_file_map.end() ? (it->second != NULL ? it->second->get_globalScope() : NULL) : NULL;  
}

void Driver<Sage>::useType(SgType * type, file_id_t file_id) {
  useType(type, getGlobalScope(file_id));
}

void Driver<Sage>::useType(SgType * type, SgScopeStatement * scope) {
  SgNamedType    * named_type = isSgNamedType(type);
  SgModifierType * mod_type   = isSgModifierType(type);
  SgPointerType  * ptr_type   = isSgPointerType(type);
  SgArrayType    * arr_type   = isSgArrayType(type);
  if (named_type != NULL) {
    SgClassType   * class_type = isSgClassType(named_type);
    SgEnumType    * enum_type  = isSgEnumType(named_type);
    SgTypedefType * tdf_type   = isSgTypedefType(named_type);

    assert(class_type != NULL || enum_type != NULL || tdf_type != NULL);

    SgGlobal * global_scope = SageInterface::getGlobalScope(scope);
    std::map<SgScopeStatement *, std::set<SgType *> >::iterator it_type_scope = p_type_scope_map.find(global_scope);
    if (it_type_scope == p_type_scope_map.end())
      it_type_scope = p_type_scope_map.insert(std::pair<SgScopeStatement *, std::set<SgType *> >(global_scope, std::set<SgType *>())).first;
    std::set<SgType *>::iterator it_type = it_type_scope->second.find(type);
    if (it_type == it_type_scope->second.end()) {
      it_type_scope->second.insert(type);

      SgDeclarationStatement * decl_stmt = named_type->get_declaration();
      assert(decl_stmt != NULL);

      if (class_type != NULL) {
        decl_stmt = decl_stmt->get_definingDeclaration();
        assert(decl_stmt != NULL);

        SgClassDeclaration * class_decl = isSgClassDeclaration(decl_stmt);
        assert(class_decl != NULL);

        SgName name = class_decl->get_name();

        SgClassDeclaration * decl_copy = isSgClassDeclaration(SageInterface::copyStatement(decl_stmt));
        assert(decl_copy != NULL);
        SgClassDeclaration * nondef_decl = SageBuilder::buildNondefiningClassDeclaration_nfi(name, decl_copy->get_class_type(), global_scope, false, NULL);

        decl_copy->set_parent(global_scope);
        decl_copy->set_scope(global_scope);
        decl_copy->set_firstNondefiningDeclaration(nondef_decl);
        decl_copy->set_definingDeclaration(decl_copy);

        nondef_decl->set_parent(global_scope);
        nondef_decl->set_scope(global_scope);
        nondef_decl->set_firstNondefiningDeclaration(nondef_decl);
        nondef_decl->set_definingDeclaration(decl_copy);

        nondef_decl->set_type(decl_copy->get_type());

//      global_scope->insert_symbol(name, new SgClassSymbol(decl_copy));
        SageInterface::prependStatement(decl_copy, global_scope);
      }
      else if (enum_type != NULL) {
        decl_stmt = decl_stmt->get_definingDeclaration();
        assert(decl_stmt != NULL);

        SgEnumDeclaration * enum_decl = isSgEnumDeclaration(decl_stmt);
        assert(enum_decl != NULL);

        SgName name = enum_decl->get_name();
        SgSymbol * enum_sym = scope->lookup_enum_symbol(name);
        if (enum_sym != NULL) return;

        SgEnumDeclaration * decl_copy = isSgEnumDeclaration(SageInterface::copyStatement(decl_stmt));
        assert(decl_copy != NULL);

        decl_copy->set_parent(global_scope);
        decl_copy->set_scope(global_scope);
//      global_scope->insert_symbol(name, new SgEnumSymbol(decl_copy));
        SageInterface::prependStatement(decl_copy, global_scope);
      }
      else if (tdf_type != NULL) {
        SgTypedefDeclaration * tdf_decl = isSgTypedefDeclaration(decl_stmt);
        assert(tdf_decl != NULL);

        SgName name = tdf_decl->get_name();
        SgSymbol * tdf_sym = scope->lookup_typedef_symbol(name);
        if (tdf_sym != NULL) return;

        useType(tdf_type->get_base_type(), scope);

        SgTypedefDeclaration * decl_copy = isSgTypedefDeclaration(SageInterface::copyStatement(decl_stmt));
        assert(decl_copy != NULL);

        decl_copy->set_parent(global_scope);
        decl_copy->set_scope(global_scope);
//      global_scope->insert_symbol(name, new SgTypedefSymbol(decl_copy));
        SageInterface::prependStatement(decl_copy, global_scope);
      }
    }
  }
  else if (mod_type != NULL) useType(mod_type->get_base_type(), scope);
  else if (ptr_type != NULL) useType(ptr_type->get_base_type(), scope);
  else if (arr_type != NULL) useType(arr_type->get_base_type(), scope);
}

/** @} */

}

