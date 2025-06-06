/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef SEARCHDIALOG_HPP
#define SEARCHDIALOG_HPP

#include <QObject>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QFont>
#include <QMap>
#include <QHash>
#include <QDialog>
#include "StelDialog.hpp"
#include "VecMath.hpp"

// pre declaration of the ui class
class Ui_searchDialogForm;
class QSortFilterProxyModel;
class QStringListModel;

struct stringLengthCompare
{
	bool operator()(const QString &s1, const QString &s2) const
	{
		return s1.length() < s2.length();
	}
};

struct recentObjectSearches
{
	int maxSize = 20;
	QStringList recentList;
};
Q_DECLARE_METATYPE(recentObjectSearches)

//! @class CompletionListModel
//! Display a list of results matching the search string, and allow to
//! tab through those selections.
class CompletionListModel : public QStringListModel
{
	Q_OBJECT

public:
	CompletionListModel(QObject* parent=Q_NULLPTR);
	CompletionListModel(const QStringList & strings, QObject* parent =Q_NULLPTR);
	~CompletionListModel() override;

	QString getSelected(void) const;
	void setValues(const QStringList&, const QStringList&);
	bool isEmpty() const {return values.isEmpty();}
	void appendValues(const QStringList&);
	void appendRecentValues(const QStringList&);
	void clearValues();

	QStringList getValues(void) { return values; }
	QStringList getRecentValues(void) { return recentValues; }
	int getSelectedIdx() { return selectedIdx; }

	// Bold recent objects
	QVariant data(const QModelIndex &index, int role) const override;

public slots:
	void selectNext();
	void selectPrevious();
	void selectFirst();

private:
	void updateText();
	int selectedIdx;
	QStringList values;
	QStringList recentValues;
};

QT_FORWARD_DECLARE_CLASS(QListWidgetItem)

//! @class SearchDialog
//! The sky object search dialog.
class SearchDialog : public StelDialog
{
	Q_OBJECT
	Q_PROPERTY(bool useSimbad       READ simbadSearchEnabled WRITE enableSimbadSearch  NOTIFY simbadUseChanged)
	Q_PROPERTY(int  simbadDist      READ getSimbadQueryDist  WRITE setSimbadQueryDist  NOTIFY simbadQueryDistChanged)
	Q_PROPERTY(int  simbadCount     READ getSimbadQueryCount WRITE setSimbadQueryCount NOTIFY simbadQueryCountChanged)
	Q_PROPERTY(bool simbadGetIds    READ getSimbadGetsIds    WRITE setSimbadGetsIds    NOTIFY simbadGetsIdsChanged)
	Q_PROPERTY(bool simbadGetSpec   READ getSimbadGetsSpec   WRITE setSimbadGetsSpec   NOTIFY simbadGetsSpecChanged)
	Q_PROPERTY(bool simbadGetMorpho READ getSimbadGetsMorpho WRITE setSimbadGetsMorpho NOTIFY simbadGetsMorphoChanged)
	Q_PROPERTY(bool simbadGetTypes  READ getSimbadGetsTypes  WRITE setSimbadGetsTypes  NOTIFY simbadGetsTypesChanged)
	Q_PROPERTY(bool simbadGetDims   READ getSimbadGetsDims   WRITE setSimbadGetsDims   NOTIFY simbadGetsDimsChanged)

public:
	//! Available coordinate systems
	enum CoordinateSystem
	{
		equatorialJ2000,
		equatorial,
		horizontal,
		galactic,
		supergalactic,
		ecliptic,
		eclipticJ2000
	};
	Q_ENUM(CoordinateSystem)

	SearchDialog(QObject* parent);
	~SearchDialog() override;
	bool eventFilter(QObject *object, QEvent *event) override;

	//! Replaces all occurrences of substrings describing Greek letters (i.e. "alpha", "beta", ...)
	//! with the actual Greek unicode characters.
	static QString substituteGreek(const QString& keyString);
	//! Returns the Greek unicode character for the specified letter string (i.e. "alpha", "beta", ...)
	static QString getGreekLetterByName(const QString& potentialGreekLetterName);
	//! URL of the default SIMBAD server (Strasbourg).
	static const char* DEF_SIMBAD_URL;

signals:
	void simbadUseChanged(bool use);
	void simbadQueryDistChanged(int dist);
	void simbadQueryCountChanged(int count);
	void simbadGetsIdsChanged(bool b);
	void simbadGetsSpecChanged(bool b);
	void simbadGetsDistChanged(bool b);
	void simbadGetsMorphoChanged(bool b);
	void simbadGetsTypesChanged(bool b);
	void simbadGetsDimsChanged(bool b);

public slots:
	void retranslate() override;
	//! On the first call with "true" populates the window contents. Also sets focus to entry line.
	void setVisible(bool v) override;
	//! This style only displays the text search field and the search button
	void setSimpleStyle();

	//! Set the current coordinate system
	void setCurrentCoordinateSystem(SearchDialog::CoordinateSystem cs)
	{
		currentCoordinateSystem = cs;
	}
	//! Set the current coordinate system from its key
	void setCurrentCoordinateSystemKey(QString key);
	//! Called when user wants to change recent search list size
	void setRecentSearchSize(int maxSize);

	void setCoordinateSystem(int csID);
	void populateCoordinateSystemsList();
	void populateCoordinateAxis();
	void populateRecentSearch();

public:
	//! Get the current coordinate system
	SearchDialog::CoordinateSystem getCurrentCoordinateSystem() const
	{
	    return currentCoordinateSystem;
	}
	//! Get the current coordinate system key
	QString getCurrentCoordinateSystemKey(void) const;
	//! Returns current max size of recent search list
	int  getRecentSearchSize () const { return recentObjectSearchesData.maxSize;}

protected:
	Ui_searchDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	void createDialogContent() override;

private slots:
	void greekLetterClicked();
	//! Query SIMBAD for data at the J2000.0 coordinates of the selected object.
	void lookupCoordinates();
	//! Signal handler
	void clearSimbadText(StelModule::StelModuleSelectAction);
	//! Called when the current simbad query status changes
	void onSimbadStatusChanged();
	//! Called when the user changed the input text
	void onSearchTextChanged(const QString& text);

	void gotoObject();
	void gotoObject(const QString& nameI18n);
	void gotoObject(const QString& nameI18n, const QString& objType);
	// for going from list views
	void gotoObject(const QModelIndex &modelIndex);
	void gotoObjectWithType(const QModelIndex &modelIndex);

	void searchListClear();
	void refreshFocus(bool state);

	//! Called when the user edit the manual position controls
	void manualPositionChanged();

	//! Whether to use SIMBAD for searches or not.
	void enableSimbadSearch(bool enable);

	//! Whether to use autofill for start of words or not.
	void enableStartOfWordsAutofill(bool enable);

	//! Whether to use sorting by string length or not
	void enableSortingByLength(bool enable);

	//! Whether to use lock position when coordinates are used or not.
	void enableLockPosition(bool enable);

        //! Whether to use automatic closing dialog when search element is selected.
        void enableAutoClosing(bool enable);

	//! Whether to show FOV center marker when coordinates are used or not.
	void enableFOVCenterMarker(bool enable);

	//! Set flagHasSelectedText as true, if search box has selected text
	void setHasSelectedFlag();

	//! Called when a SIMBAD server is selected in the list.
	void selectSimbadServer(int index);

	//! Called when new type of objects selected in list view tab
	void updateListView(int index);

	// retranslate/recreate tab
	void updateListTab();

	void showContextMenu(const QPoint &pt);

	void pasteAndGo();

	void changeTab(int index);

	void setSimbadQueryDist(int dist);
	void setSimbadQueryCount(int count);
	void setSimbadGetsIds(bool b);
	void setSimbadGetsSpec(bool b);
	void setSimbadGetsMorpho(bool b);
	void setSimbadGetsTypes(bool b);
	void setSimbadGetsDims(bool b);

	//! Update recent list's max size
	void recentSearchSizeEditingFinished();

	//! Clear recent list's data
	void recentSearchClearDataClicked();

	//! Setting coordinates of the center of screen in spinboxes (following axes of current coordinate system)
	void setCenterOfScreenCoordinates();

private:
	bool simbadSearchEnabled() const {return useSimbad;}
	int  getSimbadQueryDist () const { return simbadDist;}
	int  getSimbadQueryCount() const { return simbadCount;}
	bool getSimbadGetsIds   () const { return simbadGetIds;}
	bool getSimbadGetsSpec  () const { return simbadGetSpec;}
	bool getSimbadGetsMorpho() const { return simbadGetMorpho;}
	bool getSimbadGetsTypes () const { return simbadGetTypes;}
	bool getSimbadGetsDims  () const { return simbadGetDims;}

	class SearchDialogStaticData
	{
	public:
		//! Greek letters and strings
		QHash<QString, QString> greekLetters;

		SearchDialogStaticData() :
			greekLetters(QHash<QString,QString>({
				{"alpha",   QString(QChar(0x03B1))},
				{"beta",    QString(QChar(0x03B2))},
				{"beta",    QString(QChar(0x03B2))},
				{"gamma",   QString(QChar(0x03B3))},
				{"delta",   QString(QChar(0x03B4))},
				{"epsilon", QString(QChar(0x03B5))},

				{"zeta",    QString(QChar(0x03B6))},
				{"eta",     QString(QChar(0x03B7))},
				{"theta",   QString(QChar(0x03B8))},
				{"iota",    QString(QChar(0x03B9))},
				{"kappa",   QString(QChar(0x03BA))},

				{"lambda",  QString(QChar(0x03BB))},
				{"mu",      QString(QChar(0x03BC))},
				{"nu",      QString(QChar(0x03BD))},
				{"xi",      QString(QChar(0x03BE))},
				{"omicron", QString(QChar(0x03BF))},

				{"pi",      QString(QChar(0x03C0))},
				{"rho",     QString(QChar(0x03C1))},
				{"sigma",   QString(QChar(0x03C3))}, // second lower-case sigma shouldn't affect anything
				{"tau",     QString(QChar(0x03C4))},
				{"upsilon", QString(QChar(0x03C5))},

				{"phi",     QString(QChar(0x03C6))},
				{"chi",     QString(QChar(0x03C7))},
				{"psi",     QString(QChar(0x03C8))},
				{"omega",   QString(QChar(0x03C9))}})){}
	};
	static SearchDialogStaticData staticData;

	class SimbadSearcher* simbadSearcher;
	class SimbadLookupReply* simbadReply;
	QMap<QString, Vec3d> simbadResults; //! Simbad object name and J2000.0 coordinates
	class StelObjectMgr* objectMgr;
	class QSettings* conf;
	QStringListModel* listModel;
	QSortFilterProxyModel *proxyModel;

	//! Used when substituting text with a Greek letter.
	bool flagHasSelectedText;

	bool useStartOfWords;
	bool useLengthSorting;
	bool useLockPosition;
	bool useSimbad;
	bool useFOVCenterMarker;
	bool fovCenterMarkerState;
        bool useAutoClosing;
	//! URL of the server used for SIMBAD queries.
	QString simbadServerUrl;

	//! Properties for SIMBAD position query
	int  simbadDist;      //!< Distance from queried coordinates
	int  simbadCount;     //!< At max, retrieve so many results
	bool simbadGetIds;    //!< get all IDs for object
	bool simbadGetSpec;   //!< get spectral data
	bool simbadGetMorpho; //!< get morphological description
	bool simbadGetTypes;  //!< get object types
	bool simbadGetDims;   //!< get dimensions

	bool shiftPressed;   //!< follow Shift button status

	void populateSimbadServerList();

	// The current coordinate system
	CoordinateSystem currentCoordinateSystem;

	// Properties for "recent object searches"
	CompletionListModel* searchListModel;
	recentObjectSearches recentObjectSearchesData;
	QString recentObjectSearchesJsonPath;

	//! Add to list: called from gotoObject(const QString& nameI18n)
	void updateRecentSearchList(const QString &nameI18n);

	//! Get data from previous session
	void loadRecentSearches();
	//! Save to file after each search
	void saveRecentSearches();
	//! Shrink list if needed
	void adjustRecentList(int maxSize);
	//! Display search per user preference
	void adjustMatchesResult(QStringList &allMatches, QStringList& recentMatches, QStringList& matches, int maxNbItem);
	//! Update searches result display and reset selectedIdx = 0
	void resetSearchResultDisplay(QStringList allMatches, QStringList recentMatches);
	//! Decide if push button should be enabled
	void setPushButtonGotoSearch();
	//! Default maxNbItem when matching objects
	int defaultMaxSize = 20;
	//! Enable if recent list size is greater than 0
	void setRecentSearchClearDataPushButton();

public:
	static QString extSearchText;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param maxNbItem the maximum number of returned object names.
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object names by order of recent searches, or an empty list if nothing match
	QStringList listMatchingRecentObjects(const QString& objPrefix, int maxNbItem=20, bool useStartOfWords=false) const;
};

#endif // _SEARCHDIALOG_HPP

