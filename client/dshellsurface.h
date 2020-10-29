#ifndef DSHELLSURFACE_H
#define DSHELLSURFACE_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

struct wl_surface;

namespace DWaylandClient {

class DShellSurface;
class DShellSurfaceManagerPrivate;
class DShellSurfaceManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DShellSurfaceManager)
public:
    explicit DShellSurfaceManager(QObject *parent = nullptr);
    ~DShellSurfaceManager();

    bool isActive() const;

    DShellSurface *ensureShellSurface(struct ::wl_surface *surface);
    Q_INVOKABLE DShellSurface *registerWindow(QWindow *window);

Q_SIGNALS:
    void surfaceCreated(DShellSurface *surface, QWindow *window);
    void activeChanged();

private:
    QScopedPointer<DShellSurfaceManagerPrivate> d_ptr;
};

class DShellSurfacePrivate;
class DShellSurface : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DShellSurface)
public:
    ~DShellSurface();

    struct ::wl_surface *surface() const;
    QWindow *window() const;

    QRect geometry() const;
    QVariantMap properties() const;
    QVariant property(const QString &name) const;
    void setProperty(const QString &name, const QVariant &value);
    void requestActivate();

public Q_SLOTS:
    void sendSignal(const QString &name, const QVariant &value);

Q_SIGNALS:
    void geometryChanged(const QRect &rect);
    void propertyChanged(const QString &name, const QVariant &value);
    void notify(const QString &name, const QVariant &value);

protected:
    DShellSurface(DShellSurfacePrivate &dd, QObject *parent);

private:
    QScopedPointer<DShellSurfacePrivate> d_ptr;
    using QObject::setParent;
    friend class DShellSurfaceManager;
    friend class DShellSurfaceManagerPrivate;
};

} // DWaylandClient

#endif // DSHELLSURFACE_H
