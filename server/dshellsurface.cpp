#include "dshellsurface.h"

#include <qwayland-server-dde-shell.h>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandResource>
#include <QCoreApplication>
#include <QDebug>

#include <wayland-server.h>

#define SIGNAL_PREFIX "__DWAYLAND_SIGNAL_"

namespace DWaylandServer {

class FakeQWaylandObject : public QWaylandObject
{
public:
    FakeQWaylandObject() : QWaylandObject() {}
};

Q_GLOBAL_STATIC(FakeQWaylandObject, _d_placeholderContainer)

class DShellSurfaceManagerPrivate : public QtWaylandServer::dde_surface_manager
{
public:
    DShellSurfaceManagerPrivate(DShellSurfaceManager *qq)
        : q_ptr(qq)
    {

    }

    void dde_surface_manager_get_surface(Resource *resource, uint32_t id, struct ::wl_resource *surfaceResource) override
    {
        Q_Q(DShellSurfaceManager);

        DShellSurface *dddSurface = DShellSurface::fromResource(resource->handle);

        if (!dddSurface)
            dddSurface = new DShellSurface(q, resource->handle, surfaceResource, id);

        emit q->surfaceCreated(dddSurface);
    }

    void dde_surface_manager_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource)
    }

    wl_display *display = nullptr;
    DShellSurfaceManager *q_ptr;
    Q_DECLARE_PUBLIC(DShellSurfaceManager)
};

DShellSurfaceManager::DShellSurfaceManager(wl_display *display)
    : QWaylandCompositorExtensionTemplate(_d_placeholderContainer)
    , d_ptr(new DShellSurfaceManagerPrivate(this))
{
    d_ptr->display = display;
}

DShellSurfaceManager::DShellSurfaceManager(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate(compositor)
    , d_ptr(new DShellSurfaceManagerPrivate(this))
{

}

DShellSurfaceManager::~DShellSurfaceManager()
{

}

const wl_interface *DShellSurfaceManager::interface()
{
    return DShellSurfaceManagerPrivate::interface();
}

QByteArray DShellSurfaceManager::interfaceName()
{
    return DShellSurfaceManagerPrivate::interfaceName();
}

void DShellSurfaceManager::initialize()
{
    QWaylandCompositorExtensionTemplate::initialize();

    Q_D(DShellSurfaceManager);

    if (d->display) {
        return d->init(d->display, 1);
    }

    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor) {
        qWarning() << "Failed to find QWaylandCompositor when initializing DShellSurfaceManager";
        return;
    }

    d->init(compositor->display(), 1);
}

class DShellSurfacePrivate : public QtWaylandServer::dde_shell_surface
{
public:
    DShellSurfacePrivate(DShellSurface *qq, wl_client *client, int id, int version)
        : dde_shell_surface(client, id, version)
        , q_ptr(qq)
    {

    }

    void dde_shell_surface_get_geometry(Resource *resource) override
    {
        if (!geometry.isValid())
            return;

        send_geometry(resource->handle, geometry.x(), geometry.y(), geometry.width(), geometry.height());
    }
    void dde_shell_surface_set_property(Resource *resource, const QString &name, wl_array *value) override
    {
        Q_UNUSED(resource)
        const QByteArray data(static_cast<char*>(value->data), value->size * 4);
        QDataStream ds(data);
        QVariant vv;
        ds >> vv;

        Q_Q(DShellSurface);

        // 处理信号通知
        if (Q_UNLIKELY(name.startsWith(SIGNAL_PREFIX))) {
            Q_EMIT q->notify(name.mid(strlen(SIGNAL_PREFIX)), vv);
        } else {
            properies[name] = vv;
            Q_EMIT q->propertyChanged(name, vv);
        }
    }

    void dde_shell_surface_get_property(Resource *resource, const QString &name) override
    {
        send_property(resource, name, properies[name]);
    }

    virtual void dde_shell_surface_request_activate(Resource *resource) override
    {
        Q_UNUSED(resource)
        Q_Q(DShellSurface);

        Q_EMIT q->activationRequested();
    }

    void dde_shell_surface_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource)
        Q_Q(DShellSurface);
        Q_EMIT q->surfaceDestroyed();
    }

    void send_property(Resource *resource, const QString &name, const QVariant &value)
    {
        QByteArray data;
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << value;
        QtWaylandServer::dde_shell_surface::send_property(resource->handle, name, data);
    }

    inline void send_property(const QString &name, const QVariant &value)
    {
        send_property(resource(), name, value);
    }

    DShellSurface *q_ptr;
    DShellSurfaceManager *manager = nullptr;
    wl_resource *surfaceResource = nullptr;
    uint id = 0;
    QRect geometry;
    QVariantMap properies;

    static QWaylandSurfaceRole s_role;

    Q_DECLARE_PUBLIC(DShellSurface)
};

QWaylandSurfaceRole DShellSurfacePrivate::s_role("dde-shell-surface");

DShellSurface::DShellSurface(DShellSurfaceManager *manager, wl_resource *resource, wl_resource *surfaceResource, uint id)
    : QWaylandShellSurfaceTemplate(_d_placeholderContainer)
    , d_ptr(new DShellSurfacePrivate(this, resource->client, id, 1))
{
    Q_D(DShellSurface);

    d->manager = manager;
    d->id = id;
    d->surfaceResource = surfaceResource;

    emit surfaceChanged();
    emit idChanged();
}

DShellSurface::~DShellSurface()
{
}

wl_resource *DShellSurface::resource() const
{
    Q_D(const DShellSurface);
    return d->resource()->handle;
}

wl_resource *DShellSurface::surfaceResource() const
{
    Q_D(const DShellSurface);
    return d->surfaceResource;
}

uint DShellSurface::id() const
{
    Q_D(const DShellSurface);
    return d->id;
}

void DShellSurface::setGeometry(const QRect &rect)
{
    Q_D(DShellSurface);

    if (d->geometry == rect)
        return;

    d->geometry = rect;
    d->send_geometry(rect.x(), rect.y(), rect.width(), rect.height());
}

QVariant DShellSurface::property(const QString &name) const
{
    Q_D(const DShellSurface);

    return d->properies[name];
}

static bool qvariantComparison(const QVariant &v1, const QVariant &v2)
{
    if (v1.type() != v2.type())
        return false;

    return v1 == v2;
}

void DShellSurface::setProperty(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

    if (qvariantComparison(d->properies.value(name), value)) {
        return;
    }

    d->properies[name] = value;
    d->send_property(name, value);
}

const wl_interface *DShellSurface::interface()
{
    return DShellSurfacePrivate::interface();
}

QByteArray DShellSurface::interfaceName()
{
    return DShellSurfacePrivate::interfaceName();
}

QWaylandSurfaceRole *DShellSurface::role()
{
    return &DShellSurfacePrivate::s_role;
}

DShellSurface *DShellSurface::fromResource(wl_resource *resource)
{
    auto surfaceResource = DShellSurfacePrivate::Resource::fromResource(resource);
    if (!surfaceResource)
        return nullptr;
    return static_cast<DShellSurfacePrivate *>(surfaceResource->dde_shell_surface_object)->q_func();
}

void DShellSurface::sendSignal(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

    // 以特殊名称的属性模拟信号发送，此属性不会记录在属性map中，且不能被get，只是借用属性的通知行为
    d->send_property(QStringLiteral(SIGNAL_PREFIX) + name, value);
}

void DShellSurface::initialize()
{
    QWaylandShellSurfaceTemplate::initialize();
}

}
