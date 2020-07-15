#ifndef DSHELLSURFACE_H
#define DSHELLSURFACE_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

struct wl_surface;
class DShellSurface;
class DShellSurfaceManagerPrivate;
class DShellSurfaceManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DShellSurfaceManager)
public:
    explicit DShellSurfaceManager();
    ~DShellSurfaceManager();

    DShellSurface *ensureShellSurface(struct ::wl_surface *surface);
    Q_INVOKABLE DShellSurface *registerWindow(QWindow *window);

Q_SIGNALS:
    void surfaceCreated(DShellSurface *surface);

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

    QRect geometry() const;
    QVariant property(const QString &name) const;
    void setProperty(const QString &name, const QVariant &value);

Q_SIGNALS:
    void geometryChanged(const QRect &rect);
    void propertyChanged(const QString &name, const QVariant &value);

protected:
    DShellSurface(DShellSurfacePrivate &dd);

private:
    QScopedPointer<DShellSurfacePrivate> d_ptr;
    friend class DShellSurfaceManager;
};

#endif // DSHELLSURFACE_H
