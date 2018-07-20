#include "AST/Type.h"
#include "AST/DeclarationContext.h"
#include <iostream>

//----------------------------------------------------------------------------//
// Type
//----------------------------------------------------------------------------//

template<typename T> const T* Type::as() const {
  return dynamic_cast<const T*>(this);
}

const Type* Type::getCanonicalType() const {
  return this;
}

bool Type::isIntegerType() const {
  const Type* canonical = getCanonicalType();
  if (getKind() == Kind::TypeIdentifier) {
    return canonical->as<TypeIdentifier>()->getName() == "Integer";
  } else return false;
}

//----------------------------------------------------------------------------//
// TypeList
//----------------------------------------------------------------------------//

TypeList::TypeList(std::vector<std::shared_ptr<Type>> l) {
  if (l.size() == 0) {
    throw std::runtime_error("type list must have at least one type");
  } else if (l.size() == 1) {
    element = l[0];
    list = nullptr;
  } else {
    element = l[0];
    l.erase(l.begin());
    list = std::make_shared<TypeList>(l);
  }
}

TypeList::TypeList(std::shared_ptr<Type> e, std::shared_ptr<TypeList> l)
: element{move(e)}, list{move(l)} {}

std::vector<std::shared_ptr<TreeElement>> TypeList::getChildren() const {
  if (!list) return {element};
  else {
    auto children = list->getChildren();
    children.insert(children.begin(), element);
    return children;
  }
}

int TypeList::size() const {
  if (!list) return 1;
  else return list->size()+1;
}

std::shared_ptr<Type> TypeList::operator[] (const int index){
  if (index==0) return element;
  else return (*list)[index-1];
}


//----------------------------------------------------------------------------//
// Labeled Type
//----------------------------------------------------------------------------//

LabeledType::LabeledType(std::shared_ptr<TypeLabel> p, std::shared_ptr<Type> t)
  : label{move(p)}, type{move(t)} {}

Type::Kind LabeledType::getKind() const { return Kind::LabeledType; }

std::vector<std::shared_ptr<TreeElement>> LabeledType::getChildren() const {
  return {label, type};
}


//----------------------------------------------------------------------------//
// TypeIdentifier
//----------------------------------------------------------------------------//

std::string TypeIdentifier::getName() const {
  return token.lexeme;
}


std::shared_ptr<TupleType> TupleType::make(std::shared_ptr<TypeList> l) {
  return std::make_shared<TupleType>(l);
}

std::ostream& operator<<(std::ostream& os, Type* x) {
  if (dynamic_cast<LabeledType*>(x)) {
    auto t = dynamic_cast<LabeledType*>(x);
    os << t->label << ": " << t->type;
  } else if (dynamic_cast<TypeIdentifier*>(x)) {
    auto t = dynamic_cast<TypeIdentifier*>(x);
    os << t->getLexeme() ;
  } else if (dynamic_cast<TupleType*>(x)) {
    auto t = dynamic_cast<TupleType*>(x);
    os << "(" << t->list << ")" ;
  } else if (dynamic_cast<FunctionType*>(x)) {
    auto t = dynamic_cast<FunctionType*>(x);
    os << "(" << t->params << ") -> " << t->returns ;
  } else if (dynamic_cast<ListType*>(x)) {
    auto t = dynamic_cast<ListType*>(x);
    os << "[" << t->type << "]" ;
  } else if (dynamic_cast<MapType*>(x)) {
    auto t = dynamic_cast<MapType*>(x);
    os << "[" << t->keyType << ": " << t->valType << "]" ;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, TypeLabel* x) {
  os << x->getLexeme();
  return os;
}

std::ostream& operator<<(std::ostream& os, TypeList* x) {
  if (!x) return os;
  os << x->element;
  if (x->list) os  << ", " << x->list;
  return os;
}

bool equal(std::shared_ptr<Type> t1, std::shared_ptr<Type> t2, DeclarationContext *c) {
  if (t1->getKind() != t2->getKind()) return false;
  auto f1 = c->getFundamentalType(t1);
  auto f2 = c->getFundamentalType(t2);
  return *f1 == *f2;
}

bool equal(std::shared_ptr<TypeList> t1, std::shared_ptr<TypeList> t2, DeclarationContext *c) {
  return t1->size() == t2->size()
      && equal(t1->element, t2->element, c)
      && t1->list ? equal(t1->list, t2->list, c) : true;
}

bool operator != (const Type& l, const Type& r) {
  return !(l==r);
}

bool operator == (const Type& l, const Type& r) {
  if (l.getKind() != r.getKind()) return false;
  switch (l.getKind()) {
    case Type::Kind::LabeledType:
      return l.as<LabeledType>()->label->getLexeme() == r.as<LabeledType>()->label->getLexeme()
          && *l.as<LabeledType>()->type == *r.as<LabeledType>()->type;
    case Type::Kind::TypeIdentifier:
      return l.as<TypeIdentifier>()->getLexeme() == r.as<TypeIdentifier>()->getLexeme();
    case Type::Kind::TupleType:
      if (l.as<TupleType>()->list && r.as<TupleType>()->list)
        return *l.as<TupleType>()->list == *r.as<TupleType>()->list;
      else if (!l.as<TupleType>()->list && !r.as<TupleType>()->list)
        return true;
      else return false;
    case Type::Kind::FunctionType:
      if (l.as<FunctionType>()->params && r.as<FunctionType>()->params)
        return *l.as<FunctionType>()->params == *r.as<FunctionType>()->params;
      else if (!l.as<FunctionType>()->params && !r.as<FunctionType>()->params)
        return true;
      else return false;
    case Type::Kind::ListType:
      return *l.as<ListType>()->type == *r.as<ListType>()->type;
    case Type::Kind::MapType:
      return *l.as<MapType>()->keyType == *r.as<MapType>()->keyType
          && *l.as<MapType>()->valType == *r.as<MapType>()->valType;
    default: return false;
  }
}

bool operator == (const TypeList& l, const TypeList& r) {
  if (!l.element && !r.element) return true;
  else if (!l.element || !r.element) return false;
  else if (l.size() != r.size()) return false;

  if (!(*l.element == *r.element)) return false;
  if (l.list && r.list) return *l.list == *r.list;
  else if (!l.list && !r.list) return true;
  else return false;
}
