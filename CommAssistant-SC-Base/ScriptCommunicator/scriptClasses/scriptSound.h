#ifndef SCRIPTSOUND_H
#define SCRIPTSOUND_H

#include <QObject>
#include <QString>
#include "scriptObject.h"

class ScriptSound : public QObject, public ScriptObject
{
    Q_OBJECT
    Q_PROPERTY(QString publicScriptElements READ getPublicScriptElements CONSTANT)

public:
    explicit ScriptSound(QObject *parent, QString filename)
        : QObject(parent), m_fileName(filename)
    {
    }

    virtual QString getPublicScriptElements(void)
    {
        return MainWindow::parseApiFile("ScriptSound.api");
    }

    Q_INVOKABLE QString fileName(void) { return m_fileName; }
    Q_INVOKABLE bool isFinished(void) { return true; }
    Q_INVOKABLE void play(void) {}
    Q_INVOKABLE void stop(void) {}

private:
    QString m_fileName;
};

#endif // SCRIPTSOUND_H
