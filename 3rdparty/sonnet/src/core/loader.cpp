/**
 *
 * Copyright (C)  2003  Zack Rusin <zack@kde.org>
 * Copyright (C)  2012  Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "loader_p.h"
#include "settings_p.h"
#include "client_p.h"
#include "spellerplugin_p.h"
#include "../plugins/hunspell/hunspellclient.h"

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QLocale>
#include <QtCore/QCoreApplication>
#include <QtCore/QPluginLoader>
#include <QDebug>
#include <QtCore/QDir>

namespace Sonnet
{

class LoaderPrivate
{
public:
    Settings *settings;

    // <language, Clients with that language >
    QMap<QString, QVector<Client *> > languageClients;
    QVector<Client *> clientInstances;
    QStringList clients;

    QStringList languagesNameCache;
    QHash<QString, QSharedPointer<SpellerPlugin> > spellerCache;
};

Q_GLOBAL_STATIC(Loader, s_loader)

Loader *Loader::openLoader()
{
    if (s_loader.isDestroyed()) {
        return 0;
    }

    return s_loader();
}

Loader::Loader()
    : d(new LoaderPrivate)
{
    d->settings = new Settings(this);
    d->settings->restore();
    loadPlugins();
}

Loader::~Loader()
{
    //qDebug()<<"Removing loader : "<< this;
    delete d->settings; d->settings = 0;
    delete d;
}

SpellerPlugin *Loader::createSpeller(const QString &language,
                                     const QString &clientName) const
{
    QString pclient = clientName;
    QString plang   = language;

    if (plang.isEmpty()) {
        plang = d->settings->defaultLanguage();
    }

    const QVector<Client *> lClients = d->languageClients[plang];

    if (lClients.isEmpty()) {
        qWarning() << "No language dictionaries for the language:" << plang;
        return 0;
    }

    QVectorIterator<Client *> itr(lClients);
    while (itr.hasNext()) {
        Client *item = itr.next();
        if (!pclient.isEmpty()) {
            if (pclient == item->name()) {
                SpellerPlugin *dict = item->createSpeller(plang);
                return dict;
            }
        } else {
            //the first one is the one with the highest
            //reliability
            SpellerPlugin *dict = item->createSpeller(plang);
            return dict;
        }
    }

    return 0;
}

QSharedPointer<SpellerPlugin> Loader::cachedSpeller(const QString& language)
{
    QSharedPointer<SpellerPlugin>& speller = d->spellerCache[language];
    if (!speller) {
        speller.reset(createSpeller(language));
    }
    return speller;
}

void Loader::clearSpellerCache()
{
    d->spellerCache.clear();
}

QStringList Loader::clients() const
{
    return d->clients;
}

QStringList Loader::languages() const
{
    return d->languageClients.keys();
}

QString Loader::languageNameForCode(const QString &langCode)
{
    QString currentDictionary = langCode,   // e.g. en_GB-ize-wo_accents
            isoCode,            // locale ISO name
            variantName,         // dictionary variant name e.g. w_accents
            localizedLang,       // localized language
            localizedCountry,    // localized country
            localizedVariant;
    QByteArray variantEnglish; // dictionary variant in English

    int minusPos,          // position of "-" char
        variantCount = 0;  // used to iterate over variantList

    struct variantListType {
        const char *variantShortName;
        const char *variantEnglishName;
    };

    /*
     * This redefines the QT_TRANSLATE_NOOP3 macro provided by Qt to indicate that
     * statically initialised text should be translated so that it expands to just
     * the string that should be translated, making it possible to use it in the
     * single string construct below.
     */
#undef QT_TRANSLATE_NOOP3
#define QT_TRANSLATE_NOOP3(a, b, c) b

    const variantListType variantList[] = {
        { "40",             QT_TRANSLATE_NOOP3("Sonnet::Loader", "40", "dictionary variant") }, // what does 40 mean?
        { "60",             QT_TRANSLATE_NOOP3("Sonnet::Loader", "60", "dictionary variant") }, // what does 60 mean?
        { "80",             QT_TRANSLATE_NOOP3("Sonnet::Loader", "80", "dictionary variant") }, // what does 80 mean?
        { "ise",            QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ise suffixes", "dictionary variant") },
        { "ize",            QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ize suffixes", "dictionary variant") },
        { "ise-w_accents",  QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ise suffixes and with accents", "dictionary variant") },
        { "ise-wo_accents", QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ise suffixes and without accents", "dictionary variant") },
        { "ize-w_accents",  QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ize suffixes and with accents", "dictionary variant") },
        { "ize-wo_accents", QT_TRANSLATE_NOOP3("Sonnet::Loader", "-ize suffixes and without accents", "dictionary variant") },
        { "lrg",            QT_TRANSLATE_NOOP3("Sonnet::Loader", "large", "dictionary variant") },
        { "med",            QT_TRANSLATE_NOOP3("Sonnet::Loader", "medium", "dictionary variant") },
        { "sml",            QT_TRANSLATE_NOOP3("Sonnet::Loader", "small", "dictionary variant") },
        { "variant_0",      QT_TRANSLATE_NOOP3("Sonnet::Loader", "variant 0", "dictionary variant") },
        { "variant_1",      QT_TRANSLATE_NOOP3("Sonnet::Loader", "variant 1", "dictionary variant") },
        { "variant_2",      QT_TRANSLATE_NOOP3("Sonnet::Loader", "variant 2", "dictionary variant") },
        { "wo_accents",     QT_TRANSLATE_NOOP3("Sonnet::Loader", "without accents", "dictionary variant") },
        { "w_accents",      QT_TRANSLATE_NOOP3("Sonnet::Loader", "with accents", "dictionary variant") },
        { "ye",             QT_TRANSLATE_NOOP3("Sonnet::Loader", "with ye, modern russian", "dictionary variant") },
        { "yeyo",           QT_TRANSLATE_NOOP3("Sonnet::Loader", "with yeyo, modern and old russian", "dictionary variant") },
        { "yo",             QT_TRANSLATE_NOOP3("Sonnet::Loader", "with yo, old russian", "dictionary variant") },
        { "extended",       QT_TRANSLATE_NOOP3("Sonnet::Loader", "extended", "dictionary variant") },
        { 0, 0 }
    };

    minusPos = currentDictionary.indexOf(QLatin1Char('-'));
    if (minusPos != -1) {
        variantName = currentDictionary.right(currentDictionary.length() - minusPos - 1);
        while (variantList[variantCount].variantShortName != 0)
            if (QLatin1String(variantList[variantCount].variantShortName) == variantName) {
                break;
            } else {
                variantCount++;
            }
        if (variantList[variantCount].variantShortName != 0) {
            variantEnglish = variantList[variantCount].variantEnglishName;
        } else {
            variantEnglish = variantName.toLatin1();
        }

        localizedVariant = tr(variantEnglish.constData(), "dictionary variant");
        isoCode = currentDictionary.left(minusPos);
    } else {
        isoCode = currentDictionary;
    }

    QLocale locale(isoCode);
    localizedCountry = locale.nativeCountryName();
    localizedLang = locale.nativeLanguageName();

    if (localizedLang.isEmpty() && localizedCountry.isEmpty()) {
        return isoCode;    // We have nothing
    }

    if (!localizedCountry.isEmpty() && !localizedVariant.isEmpty()) { // We have both a country name and a variant
        return tr("%1 (%2) [%3]", "dictionary name; %1 = language name, %2 = country name and %3 = language variant name"
                 ).arg(localizedLang, localizedCountry, localizedVariant);
    } else if (!localizedCountry.isEmpty()) { // We have a country name
        return tr("%1 (%2)", "dictionary name; %1 = language name, %2 = country name"
                 ).arg(localizedLang, localizedCountry);
    } else { // We only have a language name
        return localizedLang;
    }
}

QStringList Loader::languageNames() const
{
    /* For whatever reason languages() might change. So,
     * to be in sync with it let's do the following check.
     */
    if (d->languagesNameCache.count() == languages().count()) {
        return d->languagesNameCache;
    }

    QStringList allLocalizedDictionaries;
    Q_FOREACH (const QString& langCode, languages()) {
        allLocalizedDictionaries.append(languageNameForCode(langCode));
    }
    // cache the list
    d->languagesNameCache = allLocalizedDictionaries;
    return allLocalizedDictionaries;
}

QVector<Speller::Dictionary> Loader::dictionaries() const
{
   QVector<Speller::Dictionary> dicts;

   for (int i = 0; i < d->clientInstances.count(); ++i) {
      dicts.append(d->clientInstances.at(i)->dictionaries());
   }

   return dicts;
}

Settings *Loader::settings() const
{
    return d->settings;
}

void Loader::loadPlugins()
{
    loadPlugin(QStringLiteral("Hunspell"));
}

void Loader::loadPlugin(const QString &pluginPath)
{
    Q_UNUSED(pluginPath)

    Client *client = new HunspellClient(this);
    const QStringList languages = client->languages();
    d->clients.append(client->name());
    d->clientInstances.append(client);

    Q_FOREACH (const QString &language, languages) {
        QVector<Client *> &languageClients = d->languageClients[language];

        if (languageClients.isEmpty() || client->reliability() < languageClients.first()->reliability()) {
            languageClients.append(client);    // less reliable, to the end
        } else {
            languageClients.prepend(client);    // more reliable, to the front
        }
    }
}

void Loader::changed()
{
    emit configurationChanged();
}

}

