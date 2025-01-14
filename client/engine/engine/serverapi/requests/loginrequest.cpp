#include "loginrequest.h"

#include <QJsonDocument>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

LoginRequest::LoginRequest(QObject *parent, const QString &username, const QString &password, const QString &code2fa) :
    BaseRequest(parent, RequestType::kPost),
    username_(username),
    password_(password),
    code2fa_(code2fa)
{
}

QString LoginRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray LoginRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("username", QUrl::toPercentEncoding(username_));
    postData.addQueryItem("password", QUrl::toPercentEncoding(password_));
    if (!code2fa_.isEmpty())
        postData.addQueryItem("2fa_code", QUrl::toPercentEncoding(code2fa_));
    postData.addQueryItem("session_type_id", "3");
    urlquery_utils::addAuthQueryItems(postData);
    urlquery_utils::addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl LoginRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Session");
    return url;
}

QString LoginRequest::name() const
{
    return "Login";
}

void LoginRequest::handle(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("errorCode")) {
        int errorCode = jsonObject["errorCode"].toInt();
        // 701 - will be returned if the supplied session_auth_hash is invalid. Any authenticated endpoint can
        //       throw this error.  This can happen if the account gets disabled, or they rotate their session
        //       secret (pressed Delete Sessions button in the My Account section).  We should terminate the
        //       tunnel and go to the login screen.
        // 702 - will be returned ONLY in the login flow, and means the supplied credentials were not valid.
        //       Currently we disregard the API errorMessage and display the hardcoded ones (this is for
        //       multi-language support).
        // 703 - deprecated / never returned anymore, however we should still keep this for future purposes.
        //       If 703 is thrown on login (and only on login), display the exact errorMessage to the user,
        //       instead of what we do for 702 errors.
        // 706 - this is thrown only on login flow, and means the target account is disabled or banned.
        //       Do exactly the same thing as for 703 - show the errorMessage.
        // 707 - We have been rate limited by the server.  Ask user to try later.

        if (errorCode == 701) {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return session auth hash invalid";
            sessionErrorCode_ = SessionErrorCode::kSessionInvalid;
        }
        else if (errorCode == 702) {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return bad username";
            sessionErrorCode_ = SessionErrorCode::kBadUsername;
        } else if (errorCode == 703 || errorCode == 706) {
            errorMessage_ = jsonObject["errorMessage"].toString();
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return account disabled or banned";
            sessionErrorCode_ = SessionErrorCode::kAccountDisabled;
        } else if (errorCode == 707) {
            qCDebug(LOG_SERVER_API) << "API request " + name() + " return rate limit";
            sessionErrorCode_ = SessionErrorCode::kRateLimited;
        } else {
            if (errorCode == 1340) {
                qCDebug(LOG_SERVER_API) << "API request " + name() + " return missing 2FA code";
                sessionErrorCode_ = SessionErrorCode::kMissingCode2FA;
            } else if (errorCode == 1341) {
                qCDebug(LOG_SERVER_API) << "API request " + name() + " return invalid 2FA code";
                sessionErrorCode_ = SessionErrorCode::kBadCode2FA;
            } else  {
                qCDebug(LOG_SERVER_API) << "API request " + name() + " return error";
                sessionErrorCode_ = SessionErrorCode::kUnknownError;
            }
        }
        return;
    }

    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json (data field not found)";
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
        return;
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (jsonData.contains("session_auth_hash"))
        authHash_ = jsonData["session_auth_hash"].toString();

    QString outErrorMsg;
    bool success = sessionStatus_.initFromJson(jsonData, outErrorMsg);
    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request " + name() + " incorrect json:"  << outErrorMsg;
        setNetworkRetCode(SERVER_RETURN_INCORRECT_JSON);
    }

    qCDebug(LOG_SERVER_API) << "API request " + name() + " successfully executed";
}

} // namespace server_api {
