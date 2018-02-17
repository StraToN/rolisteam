#include "tcpclient.h"
#include "channel.h"

#include <QThread>
#include <QHostAddress>

TcpClient::TcpClient(QTcpSocket* socket,QObject *parent)
    : TreeItem(parent),m_socket(socket),m_isAdmin(false)
{
    m_remainingData=0;
    m_headerRead = 0;

}
TcpClient::~TcpClient()
{
    if(nullptr != m_stateMachine)
    {
        delete m_stateMachine;
        m_stateMachine = nullptr;
    }
}

void TcpClient::setSocket(QTcpSocket* socket)
{
    m_socket = socket;
    if(nullptr != m_socket)
    {
        m_stateMachine = new QStateMachine();
        connect(m_socket,&QTcpSocket::disconnected, this, &TcpClient::socketDisconnection,Qt::QueuedConnection);
        connect(m_socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&TcpClient::socketError,Qt::QueuedConnection);

        connect(m_socket,SIGNAL(readyRead()),this,SLOT(receivingData()));
        connect(m_socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(connectionError(QAbstractSocket::SocketError)));

        connect(m_stateMachine,SIGNAL(started()),this,SIGNAL(isReady()));
        m_incomingConnection = new QState();
        m_controlConnection = new QState();
        m_authentificationServer = new QState();
        m_disconnected = new QState();

        m_connected = new QStateMachine();
        m_inChannel = new QState();
        m_wantToGoToChannel = new QState();

        m_stateMachine->addState(m_incomingConnection);
        m_stateMachine->setInitialState(m_incomingConnection);
        m_stateMachine->addState(m_controlConnection);
        m_stateMachine->addState(m_authentificationServer);
        m_stateMachine->addState(m_connected);
        m_stateMachine->addState(m_disconnected);



        m_connected->addState(m_inChannel);
        m_connected->setInitialState(m_inChannel);
        m_connected->addState(m_wantToGoToChannel);


        m_stateMachine->start();

        connect(m_incomingConnection,&QState::activeChanged,this,[=](bool b){
            qDebug() << "incomming state";
            if(b)
            {
                m_currentState = m_incomingConnection;
            }
        });
        connect(m_controlConnection,&QState::activeChanged,this,[=](bool b){
            qDebug() << "controle state";
            if(b)
            {
                m_currentState = m_controlConnection;
                emit checkServerAcceptClient(this);
            }
        });

        connect(m_authentificationServer,&QState::activeChanged,this,[=](bool b){
            qDebug() << "authentification server";
            if(b)
            {
                m_currentState = m_authentificationServer;
                emit checkServerPassword(this);
            }
        });
        connect(m_wantToGoToChannel,&QState::activeChanged,this,[=](bool b){
            if(b)
            {
                m_currentState = m_wantToGoToChannel;
            }
        });

        connect(m_disconnected,&QState::activeChanged,this,[=](bool b){
            if(b)
            {
                m_currentState = m_disconnected;
                closeConnection();
            }
        });





        m_incomingConnection->addTransition(this, SIGNAL(checkSuccess()), m_controlConnection);
        m_incomingConnection->addTransition(this,SIGNAL(checkFail()),m_disconnected);
        m_controlConnection->addTransition(this, SIGNAL(controlFail()), m_disconnected);
        m_controlConnection->addTransition(this, SIGNAL(controlSuccess()), m_authentificationServer);
        m_authentificationServer->addTransition(this, SIGNAL(serverAuthFail()), m_disconnected);
        m_authentificationServer->addTransition(this, SIGNAL(serverAuthSuccess()), m_connected);
        m_connected->addTransition(this,SIGNAL(socketDisconnection()),m_disconnected);

        m_wantToGoToChannel->addTransition(this, SIGNAL(channelAuthFail()), m_inChannel);
        m_wantToGoToChannel->addTransition(this, SIGNAL(channelAuthSuccess()), m_inChannel);
        m_inChannel->addTransition(this, SIGNAL(moveChannel()), m_wantToGoToChannel);

        emit socketInitiliazed();
    }

}

void TcpClient::startReading()
{
    QTcpSocket* socket = new QTcpSocket();
    socket->setSocketDescriptor(getSocketHandleId());
    setSocket(socket);
}

qintptr TcpClient::getSocketHandleId() const
{
    return m_socketHandleId;
}

void TcpClient::setSocketHandleId(const qintptr &socketHandleId)
{
    m_socketHandleId = socketHandleId;
}

bool TcpClient::isAdmin() const
{
    return m_isAdmin;
}

void TcpClient::setIsAdmin(bool isAdmin)
{
    m_isAdmin = isAdmin;
}

bool TcpClient::isGM() const
{
    return m_isGM;
}

void TcpClient::setIsGM(bool isGM)
{
    if(m_isGM != isGM)
    {
        m_isGM = isGM;
        emit itemChanged();
    }

}

QString TcpClient::getPlayerId()
{
    if(nullptr != m_player)
    {
        return m_player->getUuid();
    }
    return QString();
}

void TcpClient::setInfoPlayer(NetworkMessageReader* msg)
{
    if(nullptr == msg)
        return;

    if(nullptr == m_player)
        m_player = new Player();

    if(nullptr != m_player)
    {
        m_player->readFromMsg(*msg);

        /// @todo make it nicer.
        setName(m_player->getName());
        setId(m_player->getUuid());
    }
}

void TcpClient::fill(NetworkMessageWriter *msg)
{
    if(nullptr != m_player)
    {
        m_player->fill(*msg);
    }
}

bool TcpClient::isFullyDefined()
{
    if(nullptr != m_player)
    {
       return m_player->isFullyDefined();
    }
    return false;
}

void TcpClient::closeConnection()
{
    if(nullptr != m_socket)
    {
        m_socket->close();
    }
}

void TcpClient::addPlayerFeature(QString uuid, QString name, quint8 version)
{
    if(nullptr == m_player)
        return;

    if(m_player->getUuid() == uuid)
    {
        m_player->setFeature(name,version);
    }
}

void TcpClient::receivingData()
{
    if(nullptr==m_socket)
    {
       // qDebug() << "End of reading data socket null";
        return;
    }
    quint32 dataRead=0;

   // qDebug() << "current thread" << QThread::currentThread() << " thread socket" << m_socket->thread() << " object thread" << thread() << m_socket->bytesAvailable() << "sender"<< sender()<< "curent socket" << m_socket;

    while (m_socket->bytesAvailable())
    {
        if (!m_receivingData)
        {
            qint64 readDataSize = 0;
            char* tmp = (char *)&m_header;

            // To do only if there is enough data
            readDataSize = m_socket->read(tmp+m_headerRead, sizeof(NetworkMessageHeader)-m_headerRead);
            readDataSize+=m_headerRead;
            if(readDataSize!= sizeof(NetworkMessageHeader))
            {
              //m_headerRead=readDataSize;
              return;
            }
            else
            {
                m_headerRead=0;
            }
            m_buffer = new char[m_header.dataSize];
            m_remainingData = m_header.dataSize;
            emit readDataReceived(m_header.dataSize,m_header.dataSize);

        }
        // To do only if there is enough data
        dataRead = m_socket->read(&(m_buffer[m_header.dataSize-m_remainingData]), m_remainingData);
        m_dataReceivedTotal += dataRead;


        if (dataRead < m_remainingData)
        {
            m_remainingData -= dataRead;
            m_receivingData = true;
            emit readDataReceived(m_remainingData,m_header.dataSize);
            //m_socket->waitForReadyRead();
        }
        else
        {
            m_headerRead = 0;
            dataRead = 0;
            m_dataReceivedTotal = 0;
            emit readDataReceived(0,0);
            m_receivingData = false;
            m_remainingData = 0;
            forwardMessage();
        }
    }
}
bool TcpClient::isCurrentState(QState* state)
{
    return state == m_currentState;
}

QString TcpClient::getChannelPassword() const
{
    return m_channelPassword;
}

QString TcpClient::getAdminPassword() const
{
    return m_adminPassword;
}

QString TcpClient::getServerPassword() const
{
    return m_serverPassword;
}
void TcpClient::forwardMessage()
{
    QByteArray array((char*)&m_header,sizeof(NetworkMessageHeader));
    array.append(m_buffer,m_header.dataSize);
    if(!isCurrentState(m_disconnected))
    {
        if(m_header.category == NetMsg::AdministrationCategory)
        {
            NetworkMessageReader msg;
            msg.setData(array);
            readAdministrationMessages(msg);
        }
        emit dataReceived(array);
    }
}

void TcpClient::sendMessage(NetworkMessage* msg, bool deleteMsg)
{
    if((nullptr != m_socket)&&(m_socket->isWritable()))
    {
        NetworkMessageHeader* data = msg->buffer();
        qint64 dataSend = m_socket->write((char*)data,data->dataSize+sizeof(NetworkMessageHeader));
        if(-1 == dataSend)
        {
            if(m_socket->state() != QAbstractSocket::ConnectedState)
            {
                emit socketDisconnection();
            }
        }
    }
    if(deleteMsg)
    {
        delete msg;
    }
}
void TcpClient::connectionError(QAbstractSocket::SocketError error)
{
    if(nullptr!=m_socket)
        qWarning() << m_socket->errorString() << error;
}

void TcpClient::sendEvent(TcpClient::ConnectionEvent event)
{
    if(nullptr != m_player)
        qDebug() << "server connection to "<<m_player->getName() << "recieve event:" <<event;
    switch (event)
    {
    case CheckSuccessEvent:
        emit checkSuccess();
        break;
    case CheckFailEvent:
        emit checkFail();
        break;
    case ControlFailEvent:
        emit controlFail();
        break;
    case ControlSuccessEvent:
        emit controlSuccess();
        break;
    case ServerAuthDataReceivedEvent:
        emit serverAuthDataReceived();
        break;
    case ServerAuthFailEvent:
        emit serverAuthFail();
        break;
    case ServerAuthSuccessEvent:
        emit serverAuthSuccess();
        break;
    case AdminAuthSuccessEvent:
        emit adminAuthSucceed();
        m_isAdmin = true;
        break;
    case AdminAuthFailEvent:
        emit adminAuthFailed();
        m_isAdmin = false;
        break;
    case ChannelAuthSuccessEvent:
        emit channelAuthSuccess();
        break;
    case ChannelAuthFailEvent:
        emit channelAuthFail();
        break;
    case MoveChanEvent:
        emit moveChannel();
        break;
    case ChannelChanged:
        sendOffChannelChanged();
        break;
    default:
        qDebug() << "sendEvent Unkown event";
        break;
    }
}
void TcpClient::sendOffChannelChanged()
{
    NetworkMessageWriter msg(NetMsg::AdministrationCategory,NetMsg::MovedIntoChannel);
    sendMessage(&msg,false);
}
void TcpClient::readAdministrationMessages(NetworkMessageReader& msg)
{
    qDebug() << "admin message - action: "<< msg.action() <<"category:" <<msg.category();
    switch (msg.action())
    {
        case NetMsg::ConnectionInfo:
            m_serverPassword = msg.string32();
            qDebug() <<"server password" <<m_serverPassword;
            setInfoPlayer(&msg);
            emit checkServerPassword(this);
        break;
        case NetMsg::ChannelPassword:
            m_channelPassword = msg.string32();
        break;
        case NetMsg::MoveChannel:
            m_wantedChannel = msg.string32();
            m_channelPassword = msg.string32();
        break;
        case NetMsg::AdminPassword:
            m_adminPassword = msg.string32();
            emit checkAdminPassword(this);
        break;
        default:
            break;
    }
}

Channel *TcpClient::getParentChannel() const
{
    return dynamic_cast<Channel*>(getParentItem());
}

void TcpClient::setParentChannel(Channel *parent)
{
    setParentItem(parent);
}
QTcpSocket* TcpClient::getSocket()
{
    return m_socket;
}
int TcpClient::indexOf(TreeItem*)
{
    return -1;
}

void TcpClient::readFromJson(QJsonObject &json)
{
    m_isGM = json["gm"].toBool();
    setName(json["name"].toString());
    setId(json["id"].toString());
}

void TcpClient::writeIntoJson(QJsonObject &json)
{
    json["type"]="client";
    json["name"]=m_name;
    json["id"]=m_id;
    json["gm"]=m_isGM;
}
QString TcpClient::getIpAddress()
{
    if(nullptr != m_socket)
    {
        return m_socket->peerAddress().toString();
    }
    return {};
}

QString TcpClient::getWantedChannel()
{
    return m_wantedChannel;
}
bool TcpClient::isConnected() const
{
    if(!m_socket.isNull())
        return (m_socket->isValid() & (m_socket->state() == QAbstractSocket::ConnectedState));
    else
        return false;
}
