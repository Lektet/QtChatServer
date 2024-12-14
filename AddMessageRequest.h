#ifndef ADDMESSAGEREQUEST_H
#define ADDMESSAGEREQUEST_H

#include "ChatDataRequest.h"

#include "NewChatMessageData.h"

class AddMessageRequest : public ChatDataRequest
{
public:
    AddMessageRequest(int id, const NewChatMessageData& messageData);

    NewChatMessageData getMessageData() const;

private:
    NewChatMessageData message;
};

#endif // ADDMESSAGEREQUEST_H
