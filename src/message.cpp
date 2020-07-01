#include "message.h"

namespace jsonrpc{

Message::Message(const QJsonObject& message)
{
    if(message.value("jsonrpc") != "2.0"){
        throw "Invalid message";
    }

    id = message.value("id");
    if( !(id.isUndefined() || id.isNull() || id.isString() || id.isDouble()) ){
        throw "Invalid message";
    }
}

QJsonValue Message::getId() const
{
    return id;
}

bool Message::hasId() const
{
    return !(id.isUndefined() || id.isNull());
}

QJsonObject Message::toJson() const
{

}

Message::Message()
{

}

Message::Message(const QJsonValue& id)
{

}

}
