#include "ChatDataRequest.h"

ChatDataRequest::ChatDataRequest(int actionId, ChatDataRequestType actionType) :
    id(actionId),
    type(actionType)
{

}

int ChatDataRequest::getActionId() const
{
    return id;
}

ChatDataRequestType ChatDataRequest::getActionType() const
{
    return type;
}
