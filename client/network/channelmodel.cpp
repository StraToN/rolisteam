#include "channelmodel.h"

#include "treeitem.h"
#include "tcpclient.h"

#include <QSettings>
#include <QList>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef QT_WIDGETS_LIB
#include <QApplication>
#include <QStyle>
#endif

#include "channel.h"

ChannelModel::ChannelModel()
{
    //m_root = new Channel();
}

QModelIndex ChannelModel::index(int row, int column, const QModelIndex &parent) const
{
    if(row<0)
        return QModelIndex();

    TreeItem* childItem = nullptr;
    if (!parent.isValid())
    {
       // qDebug() << "invalid parent";
        //childItem = m_root->getChildAt(row);
        childItem = m_root.at(row);
    }
    else
    {
       // qDebug() << "valid parent";
        TreeItem* parentItem = static_cast<TreeItem*>(parent.internalPointer());
        childItem = parentItem->getChildAt(row);
    }

    if (childItem)
    {
        return createIndex(row, column, childItem);
    }
    else
        return QModelIndex();
}

QVariant ChannelModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    TreeItem* tmp = static_cast<TreeItem*>(index.internalPointer());
   // qDebug() << "[data channelmodel]"<<  tmp;
    if(tmp!=nullptr)
    {
        if((role == Qt::DisplayRole)||(Qt::EditRole==role))
        {
            return tmp->getName();
        }
        #ifdef QT_WIDGETS_LIB
        else if(role == Qt::DecorationRole)
        {
            if(!tmp->isLeaf())
            {
                QStyle* style = qApp->style();

                return style->standardIcon(QStyle::SP_DirIcon);
                //return QIcon(":/resources/icons/folder.png");
            }
        }
        #endif
    }
    return QVariant();
}

bool ChannelModel::setData(const QModelIndex &index, const QVariant &value, int )
{
    if(!index.isValid())
        return false;

    TreeItem* tmp = static_cast<TreeItem*>(index.internalPointer());
    if(tmp!=nullptr)
    {
        tmp->setName(value.toString());
        return true;
    }
    return false;
}

QModelIndex ChannelModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TreeItem* childItem = static_cast<TreeItem*>(child.internalPointer());
    if(nullptr!=childItem)
    {
        TreeItem* parentItem = childItem->getParentItem();
       // qDebug() << "parent Item" << parentItem << childItem->childCount()<< childItem;

        if(m_root.contains(childItem))
        {
            return QModelIndex();
        }
        if(m_root.contains(parentItem))
        {
            return createIndex(m_root.indexOf(parentItem), 0, parentItem);
        }
        return createIndex(parentItem->rowInParent(), 0, parentItem);
    }
    return QModelIndex();
}

int ChannelModel::rowCount(const QModelIndex &parent) const
{
    int result = 0;
    if(!parent.isValid())
    {
        result = m_root.size();
    }
    else
    {
        TreeItem* item = static_cast<TreeItem*>(parent.internalPointer());
        if(NULL!=item)
        {
            result = item->childCount();
        }
    }
   // qDebug() << "[Number of child]"<< result << parent.isValid();
    return result;
}

int ChannelModel::columnCount(const QModelIndex &) const
{
    return 1;
}

int ChannelModel::addChannel(QString name, QString password)
{
    Channel* chan = new Channel(name);
    chan->setPassword(password);
    QModelIndex index;
    addChannelToChannel(chan,index);
    return m_root.indexOf(chan);
}
QModelIndex ChannelModel::addChannelToChannel(Channel* channel,QModelIndex& parent)
{
    int row = -1;
    if(!parent.isValid())
    {
       beginInsertRows(parent,m_root.size(),m_root.size());
       m_root.append(channel);
       endInsertRows();
       row = m_root.size()-1;
    }
    else
    {
        Channel* item = static_cast<Channel*>(parent.internalPointer());
        if(nullptr!=item)
        {
            beginInsertRows(parent,item->childCount(),item->childCount());
            item->addChild(channel);
            endInsertRows();
            row = item->childCount()-1;
        }
    }


    return index(row,0,parent);
}

Qt::ItemFlags ChannelModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if(isAdmin())
    {
        return Qt::ItemIsEnabled  | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
    else
    {
        return Qt::ItemIsEnabled  | Qt::ItemIsSelectable;//| Qt::ItemIsEditable
    }
}
bool ChannelModel::hasChildren ( const QModelIndex & parent  ) const
{

    if(!parent.isValid())//root
    {
        return  !m_root.isEmpty();//==0?false:true;
    }
    else
    {
        TreeItem* childItem = static_cast<TreeItem*>(parent.internalPointer());
        if(childItem->childCount()==0)
            return false;
        else
            return true;
    }
}
bool ChannelModel::addConnectionToDefaultChannel(TcpClient* client)
{
    if(m_defaultChannel.isEmpty())
    {
        if(!m_root.isEmpty())
        {
            auto item = m_root.at(0);
            if(nullptr != item)
            {
                m_defaultChannel = item->getId();
            }
        }
        else
        {
            return false;
        }
    }

    return addConnectionToChannel(m_defaultChannel,client);

}

bool ChannelModel::addConnectionToChannel(QString chanId, TcpClient* client)
{
    bool found=false;
    for(auto item : m_root)
    {
        //TreeItem* item = m_root->getChildAt(i);
        if(nullptr != item)
        {
            found = item->addChildInto(chanId,client);
        }
    }
    return found;
}
void ChannelModel::readDataJson(const QJsonObject& obj)
{
    beginResetModel();
    for(auto item : m_root)
    {
        item->clear();
    }
    m_root.clear();
    QJsonArray channels = obj["channel"].toArray();
    for(auto channelJson : channels)
    {
        Channel* tmp = new Channel();
        QJsonObject obj = channelJson.toObject();
        tmp->setParentItem(nullptr);
        tmp->readFromJson(obj);
        m_root.append(tmp);
    }
    endResetModel();


    ///////////
  /*  qDebug() << "m_root:" << m_root->childCount();

    for(auto item : m_root)
    {
        qDebug() << "child:" << item->childCount();
    }*/
}

void ChannelModel::writeDataJson(QJsonObject& obj)
{
    QJsonArray array;
    for(auto item : m_root) //int i = 0; i< m_root->childCount(); ++i)
    {
        //auto item = m_root->getChildAt(i);
        if(nullptr != item)
        {
            QJsonObject jsonObj;
            item->writeIntoJson(jsonObj);
            array.append(jsonObj);
        }
    }
    obj["channel"]=array;
}

void ChannelModel::readSettings()
{
    QSettings settings("Rolisteam","roliserver");

    QJsonDocument doc= QJsonDocument::fromVariant(settings.value("channeldata",""));

    QJsonObject obj = doc.object();

    readDataJson(obj);
}


void ChannelModel::writeSettings()
{
    QSettings settings("Rolisteam","roliserver");

    QJsonDocument doc;

    QJsonObject obj;

    writeDataJson(obj);

    doc.setObject(obj);
    settings.setValue("channeldata",doc);


}

void ChannelModel::kick(QString id)
{
    for(auto item : m_root)
    {
        if(nullptr != item)
        {
            item->kick(id);
        }
    }
}

bool ChannelModel::isAdmin() const
{
    return m_admin;
}

void ChannelModel::setAdmin(bool admin)
{
    m_admin = admin;
}




