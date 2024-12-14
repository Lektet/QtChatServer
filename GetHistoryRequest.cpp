#include "GetHistoryRequest.h"

#include "ChatDataRequestType.h"

GetHistoryRequest::GetHistoryRequest(int actionId) :
    ChatDataRequest(actionId, ChatDataRequestType::GetHistory)
{

}
