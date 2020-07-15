#include "dshellsurface.h"

#include <qwayland-server-dde-shell.h>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandResource>

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
        QWaylandSurface *surface = QWaylandSurface::fromResource(surfaceResource);

        if (ddeSurfaces.contains(id)) {
            wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT,
                                   "Given id, %d, is already assigned to wl_surface@%d", id,
                                   wl_resource_get_id(ddeSurfaces[id]->surface()->resource()));
            return;
        }

        if (!surface->setRole(DShellSurface::role(), resource->handle, 1))
            return;

        QWaylandResource ddeSurfaceResource(wl_resource_create(resource->client(), &dde_shell_surface_interface,
                                                               wl_resource_get_version(resource->handle), id));

        emit q->surfaceRequested(surface, id, ddeSurfaceResource);

        DShellSurface *dddSurface = DShellSurface::fromResource(ddeSurfaceResource.resource());

        if (!dddSurface)
            dddSurface = new DShellSurface(q, surface, id, ddeSurfaceResource);

        ddeSurfaces.insert(id, dddSurface);

        emit q->surfaceCreated(dddSurface);
    }

    wl_display *display = nullptr;
    QHash<uint, DShellSurface*> ddeSurfaces;
    DShellSurfaceManager *q_ptr;
    Q_DECLARE_PUBLIC(DShellSurfaceManager)
};

DShellSurfaceManager::DShellSurfaceManager(wl_display *display)
    : d_ptr(new DShellSurfaceManagerPrivate(this))
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
    DShellSurfacePrivate(DShellSurface *qq)
        : q_ptr(qq)
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
        ds << vv;
        properies[name] = vv;

        Q_EMIT q_func()->propertyChanged(name, vv);
    }
    void dde_shell_surface_get_property(Resource *resource, const QString &name) override
    {
        send_property(resource, name, properies[name]);
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
    QWaylandSurface *surface = nullptr;
    uint id = 0;
    QRect geometry;
    QVariantMap properies;

    static QWaylandSurfaceRole s_role;

    Q_DECLARE_PUBLIC(DShellSurface)
};

QWaylandSurfaceRole DShellSurfacePrivate::s_role("dde-shell-surface");

DShellSurface::DShellSurface(DShellSurfaceManager *manager, QWaylandSurface *surface, uint id, const QWaylandResource &resource)
    : QWaylandShellSurfaceTemplate(surface)
    , d_ptr(new DShellSurfacePrivate(this))
{
    Q_D(DShellSurface);

    d->manager = manager;
    d->surface = surface;
    d->id = id;

    d->init(resource.resource());

    emit surfaceChanged();
    emit idChanged();
}

DShellSurface::~DShellSurface()
{

}

QWaylandSurface *DShellSurface::surface() const
{
    Q_D(const DShellSurface);
    return d->surface;
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

void DShellSurface::setProperty(const QString &name, const QVariant &value)
{
    Q_D(DShellSurface);

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

void DShellSurface::initialize()
{
    QWaylandShellSurfaceTemplate::initialize();
}
