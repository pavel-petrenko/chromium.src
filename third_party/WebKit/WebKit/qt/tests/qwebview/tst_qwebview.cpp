/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2009 Torch Mobile Inc.
    Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qtest.h>

#include <qpainter.h>
#include <qwebview.h>
#include <qwebpage.h>
#include <qnetworkrequest.h>
#include <qdiriterator.h>
#include <qwebkitversion.h>
#include <qwebframe.h>

class tst_QWebView : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void renderHints();
    void getWebKitVersion();

    void reusePage_data();
    void reusePage();
};

// This will be called before the first test function is executed.
// It is only called once.
void tst_QWebView::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_QWebView::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_QWebView::init()
{
}

// This will be called after every test function.
void tst_QWebView::cleanup()
{
}

void tst_QWebView::renderHints()
{
    QWebView webView;

    // default is only text antialiasing
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(!(webView.renderHints() & QPainter::SmoothPixmapTransform));
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::Antialiasing, true);
    QVERIFY(webView.renderHints() & QPainter::Antialiasing);
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(!(webView.renderHints() & QPainter::SmoothPixmapTransform));
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::Antialiasing, false);
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(!(webView.renderHints() & QPainter::SmoothPixmapTransform));
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QVERIFY(!(webView.renderHints() & QPainter::Antialiasing));
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(webView.renderHints() & QPainter::SmoothPixmapTransform);
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));

    webView.setRenderHint(QPainter::SmoothPixmapTransform, false);
    QVERIFY(webView.renderHints() & QPainter::TextAntialiasing);
    QVERIFY(!(webView.renderHints() & QPainter::SmoothPixmapTransform));
    QVERIFY(!(webView.renderHints() & QPainter::HighQualityAntialiasing));
}

void tst_QWebView::getWebKitVersion()
{
    QVERIFY(qWebKitVersion().toDouble() > 0);
}

void tst_QWebView::reusePage_data()
{
    QTest::addColumn<QString>("html");
    QTest::newRow("WithoutPlugin") << "<html><body id='b'>text</body></html>";
    QTest::newRow("WindowedPlugin") << QString("<html><body id='b'>text<embed src='resources/test.swf'></embed></body></html>");
    QTest::newRow("WindowlessPlugin") << QString("<html><body id='b'>text<embed src='resources/test.swf' wmode=\"transparent\"></embed></body></html>");
}

void tst_QWebView::reusePage()
{
    QDir::setCurrent(SRCDIR);

    QFETCH(QString, html);
    QWebView* view1 = new QWebView;
    QPointer<QWebPage> page = new QWebPage;
    view1->setPage(page);
    page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    QWebFrame* mainFrame = page->mainFrame();
    mainFrame->setHtml(html, QUrl::fromLocalFile(QDir::currentPath()));
    if (html.contains("</embed>")) {
        // some reasonable time for the PluginStream to feed test.swf to flash and start painting
        QTest::qWait(2000);
    }

    view1->show();
    QTest::qWait(2000);
    delete view1;
    QVERIFY(page != 0); // deleting view must not have deleted the page, since it's not a child of view

    QWebView *view2 = new QWebView;
    view2->setPage(page);
    view2->show(); // in Windowless mode, you should still be able to see the plugin here
    QTest::qWait(2000);
    delete view2;

    delete page; // must not crash

    QDir::setCurrent(QApplication::applicationDirPath());
}

QTEST_MAIN(tst_QWebView)
#include "tst_qwebview.moc"

