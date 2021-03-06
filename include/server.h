#ifndef JSONRPCSERVER_H
#define JSONRPCSERVER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <cassert>
#include <functional>
#include <iostream>
#include <type_traits>

#include "connection.h"

using namespace std;
namespace jsonrpc {

// Intermediate class that the JsonRpcServer inherits from, as template classes cannot use the QObject macro
// All signals and slots have to be defined here
class IntermediateServer: public QObject {
  Q_OBJECT
 public:
  explicit IntermediateServer(QObject* parent = nullptr): QObject(parent) {}
 public Q_SLOTS:
  virtual void onNewConnection() {}
};

// Only allow templates for classes that are based on QObject
// Empty class for everything not based on QObject
template<class T, bool = std::is_base_of<QObject, T>::value>
class Server: IntermediateServer {};

// Class for everything based on QObject
template<class T>
class Server<T, true>: public IntermediateServer {
 private:
  QWebSocketServer* webSocketServer;
  QList<Connection*> connections;
  std::function<T*()> constructorFunction;
  int port;

 public:
  explicit Server(int port, QObject* parent = nullptr);
  ~Server();
  void startListening();
  void onNewConnection() override;

  template<typename... Args>
  void setConstructorArguments(Args&&... args);
};

// Template implementation

template<class T>
Server<T, true>::Server(int port, QObject* parent): IntermediateServer(parent), port(port) {
  webSocketServer = new QWebSocketServer(QStringLiteral("SERVERNAME"), QWebSocketServer::NonSecureMode, this);
}

template<class T>
Server<T, true>::~Server() {
  webSocketServer->close();
}

template<class T>
void Server<T, true>::startListening() {
  if(!constructorFunction) {
    if constexpr(is_constructible<T>::value) {
      constructorFunction = []() {
        return new T();
      };
    } else {
      assert(is_constructible<T>::value);
    }
  }
  if(webSocketServer->listen(QHostAddress::Any, port)) {
    qDebug() << "Server listening on " << QHostAddress::Any << ":" << port;
    connect(webSocketServer, &QWebSocketServer::newConnection, this, &Server<T>::onNewConnection);
  } else {
    qDebug() << "Error opening server on " << QHostAddress::Any << ":" << port;
  }
}

template<class T>
void Server<T, true>::onNewConnection() {
  QWebSocket* webSocket = webSocketServer->nextPendingConnection();
  T* target = constructorFunction();
  Connection* connection = new Connection(target, this);
  ((QObject*)target)->setParent(connection);
  ((QObject*)webSocket)->setParent(connection);

  connect(webSocket, &QWebSocket::disconnected, connection, &Connection::disconnect);
  connect(connection, &Connection::sendMessage, webSocket, [webSocket](const QString& message) {
    webSocket->sendBinaryMessage(message.toUtf8());
  });
  connect(webSocket, &QWebSocket::textMessageReceived, connection, &Connection::receiveMessage);
  connect(webSocket, &QWebSocket::binaryMessageReceived, connection, &Connection::receiveMessage);

  connections.push_back(connection);
}

template<class T>
template<typename... Args>
void Server<T, true>::setConstructorArguments(Args&&... args) {
  // Create a tuple containing the arguments
  auto argsTuple = std::tuple<Args...>(args...);
  // Create a lambda that captures the argsTuple and uses it to create a new T
  std::function<T*()> constructorFunction = [argsTuple]() {
    T* result = std::apply(
        [](auto&&... args) {
          return new T(args...);
        },
        argsTuple);
    return result;
  };
  // Set the constructor function
  this->constructorFunction = constructorFunction;
}

}  // namespace jsonrpc

#endif  // JSONRPCSERVER_H
