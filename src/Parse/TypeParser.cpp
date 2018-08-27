#include "Parse/Parser.h"
#include "AST/Type.h"

#include "Basic/CompilerException.h"

#include <memory>

using namespace std;


const Type* Parser::parseType() {
  switch(token_.type()) {
    case Token::l_paren: return parseTupleOrFunctionType();
    case Token::identifier: return parseTypeIdentifier();
    case Token::l_brace: return parseStructType();
    case Token::operator_id:
      if (token_.lexeme() == StringRef{"*"}) return parsePointerType();
      else if (token_.lexeme() == StringRef{"&"}) return parseReferenceOrSliceType();
    case Token::l_square: return parseListOrMapType();
    default: throw CompilerException(token_.location(),  "error: unable to parse type");
  }
}

const Type* Parser::parseTypeIdentifier() {
  auto token = expectToken(Token::identifier, "type identifier");
  if (token.lexeme() == StringRef{"i64"}) return IntegerType::getInstance();
  else if (token.lexeme()== StringRef{"bool"}) return BooleanType::getInstance();
  else if (token.lexeme()== StringRef{"f64"}) return DoubleType::getInstance();
  else if (token.lexeme()== StringRef{"char"}) return CharacterType::getInstance();
  else {
    std::stringstream ss;
    ss << "error: unrecognized type identifier " << token.lexeme();
    throw CompilerException(token.location(), ss.str());
  }
}

std::vector<const Type*> Parser::parseTupleTypeElementList() {
  std::vector<const Type*> elements;
  elements.push_back(parseType());
  while (consumeToken(Token::comma)) {
    elements.push_back(parseType());
  }
  return elements;
}

const TupleType* Parser::parseTupleType() {
  expectToken(Token::l_paren, "left parenthesis");
  std::vector<const Type*> list;
  if (token_.isNot(Token::r_paren)) {
    list = parseTupleTypeElementList();
  }
  expectToken(Token::r_paren, "right parenthesis");
  return TupleType::getInstance(std::move(list));
}

const FunctionType* Parser::parseFunctionType() {
  expectToken(Token::l_paren, "left parenthesis");
  std::vector<const Type*> list;
  if (token_.isNot(Token::r_paren)){
    list = parseTupleTypeElementList();
  }
  expectToken(Token::r_paren, "right parenthesis");
  if (!consumeOperator("->")) throw CompilerException(token_.location(),  "error: expected ->");
  auto type = parseType();
  return FunctionType::getInstance(std::move(list), type);
}

const Type* Parser::parseTupleOrFunctionType() {
  expectToken(Token::l_paren, "left parenthesis");
  std::vector<const Type*> list;
  if (token_.isNot(Token::r_paren)) {
    list = parseTupleTypeElementList();
  }
  expectToken(Token::r_paren, "right parenthesis");
  if (!consumeOperator("->")) return TupleType::getInstance(std::move(list));
  auto type = parseType();
  return FunctionType::getInstance(std::move(list), type);
}

const ListType* Parser::parseListType() {
  expectToken(Token::l_square, "left square bracket");
  auto type = parseType();
  expectToken(Token::comma, "comma");
  auto size = parseIntegerExpr();
  expectToken(Token::r_square, "right square bracket");
  return ListType::getInstance(type, size->getInt());
}

const PointerType* Parser::parsePointerType() {
  expectToken(Token::operator_id, "*");
  auto type = parseType();
  return PointerType::getInstance(type);
}

const Type* Parser::parseReferenceOrSliceType() {
  expectToken(Token::operator_id, "&");
  if (consumeToken(Token::l_square)) {
    auto type = parseType();
    expectToken(Token::r_square, "]");
    return SliceType::getInstance(type);
  } else {
    auto type = parseType();
    return ReferenceType::getInstance(type);
  }
}

const MapType* Parser::parseMapType() {
  expectToken(Token::l_square, "left square bracket");
  auto keyType = parseType();
  expectToken(Token::colon, "colon");
  auto valueType = parseType();
  expectToken(Token::r_square, "right square bracket");
  return MapType::getInstance(keyType, valueType);
}


const StructType* Parser::parseStructType() {
  std::map<std::string, const Type*> fields;
  expectToken(Token::l_brace, "left brace");
  expectToken(Token::new_line, "newline");
  while(!token_.is(Token::r_brace)) {
    auto field_name = parseIdentifier();
    expectToken(Token::colon, "colon");
    auto field_type = parseType();
    expectToken(Token::new_line, "newline");
    if (fields.find(field_name->lexeme().str()) != fields.end()) {
      throw CompilerException(field_name->lexeme().start, "error: duplicate field name");
    } else {
      fields[field_name->lexeme().str()] = field_type;
    }
  }
  expectToken(Token::r_brace, "right brace");
  return StructType::getInstance(std::move(fields));
}

const Type* Parser::parseListOrMapType() {
  expectToken(Token::l_square, "left square bracket");
  auto keyType = parseType();
  if (consumeToken(Token::colon)) {
    auto valueType = parseType();
    expectToken(Token::r_square, "right square bracket");
    return MapType::getInstance(keyType, valueType);
  }
  expectToken(Token::comma, "comma");
  auto size = parseIntegerExpr();
  expectToken(Token::r_square, "right square bracket");
  return ListType::getInstance(keyType, size->getInt());
}
