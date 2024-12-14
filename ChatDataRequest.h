#ifndef CHATDATAREQUEST_H
#define CHATDATAREQUEST_H

enum class ChatDataRequestType;

class ChatDataRequest
{
public:
    ChatDataRequest(int actionId, ChatDataRequestType actionType);

    int getActionId() const;
    ChatDataRequestType getActionType() const;

private:
    int id;
    ChatDataRequestType type;
};

#endif // CHATDATAREQUEST_H
