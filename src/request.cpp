#include "request.h"

#include <exceptions.h>

namespace jsonrpc {

Request::Request(const QJsonObject& message): Message(message) {
  QJsonValue jsonMethodName = message.value("method");
  if(!jsonMethodName.isString()) {
    throw exceptions::InvalidRequest("the method is no string.");
  }
  methodName = jsonMethodName.toString();

  if(!buildArguments(message.value("params"))) {
    throw exceptions::InvalidRequest("params seems to be invalid.");
  }
}

QString Request::getMethodName() const {
  return methodName;
}

QList<QJsonValue> Request::getArguments() const {
  return arguments;
}

QJsonObject Request::toJson() const {
  QJsonObject requestMessageJson = Message::toJson();

  requestMessageJson.insert("method", methodName);

  if(arguments.size() != 0) {
    QJsonArray requestArguments;
    for(const QJsonValue& argument : arguments) {
      requestArguments.append(argument);
    }
    requestMessageJson.insert("params", requestArguments);
  }

  return requestMessageJson;
}

Request::Request(const QString& methodName, const QList<QJsonValue>& arguments): Message(), methodName(methodName), arguments(arguments) {}

Request::Request(const QJsonValue& id, const QString& methodName, const QList<QJsonValue>& arguments)
    : Message(id), methodName(methodName), arguments(arguments) {}

bool Request::buildArguments(const QJsonValue& passedArguments) {
  switch(passedArguments.type()) {
    case QJsonValue::Array:
      for(QJsonValue val : passedArguments.toArray()) {
        arguments.append(val);
      }
      break;
    case QJsonValue::Undefined:
      // It is valid to have no parameters
      break;
    case QJsonValue::Object:
      // TODO parameter passing by name is also valid, but not implemented yet
      qDebug() << "Parameter passing by name is not implemented.";
      return false;
    default:
      return false;
  }

  return true;
}

}  // namespace jsonrpc
