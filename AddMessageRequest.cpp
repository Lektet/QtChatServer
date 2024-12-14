#include "AddMessageRequest.h"

#include "ChatDataRequestType.h"

AddMessageRequest::AddMessageRequest(int id, const NewChatMessageData &messageData):
    ChatDataRequest(id, ChatDataRequestType::AddMessage),
    message(messageData)
{

}

NewChatMessageData AddMessageRequest::getMessageData() const
{
    return message;
}
