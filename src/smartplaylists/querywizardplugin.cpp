/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "querywizardplugin.h"
#include "searchtermwidget.h"
#include "ui_querysearchpage.h"
#include "ui_querysortpage.h"

#include <QWizardPage>

namespace smart_playlists {

QueryWizardPlugin::QueryWizardPlugin(LibraryBackend* library, QObject* parent)
  : WizardPlugin(library, parent),
    search_page_(NULL)
{
}

QueryWizardPlugin::~QueryWizardPlugin() {
}

QString QueryWizardPlugin::name() const {
  return tr("Library search");
}

QString QueryWizardPlugin::description() const {
  return tr("Find songs in your library that match the criteria you specify.");
}

int QueryWizardPlugin::CreatePages(QWizard* wizard) {
  // Create the UI
  search_page_ = new SearchPage(wizard);

  QWizardPage* sort_page = new QWizardPage(wizard);
  sort_ui_.reset(new Ui_SmartPlaylistQuerySortPage);
  sort_ui_->setupUi(sort_page);

  sort_ui_->limit_value->setValue(Generator::kDefaultLimit);

  connect(search_page_->ui_->type, SIGNAL(currentIndexChanged(int)), SLOT(SearchTypeChanged()));

  // Create the new search term widget
  search_page_->new_term_ = new SearchTermWidget(library_, search_page_);
  search_page_->new_term_->SetActive(false);
  connect(search_page_->new_term_, SIGNAL(Clicked()), SLOT(AddSearchTerm()));

  // Add an empty initial term
  search_page_->layout_ = static_cast<QVBoxLayout*>(search_page_->ui_->terms_group->layout());
  search_page_->layout_->addWidget(search_page_->new_term_);
  AddSearchTerm();

  // Add the preview widget at the bottom of the search terms page
  QVBoxLayout* terms_page_layout = static_cast<QVBoxLayout*>(search_page_->layout());
  terms_page_layout->addStretch();
  search_page_->preview_ = new SearchPreview(search_page_);
  search_page_->preview_->set_library(library_);
  terms_page_layout->addWidget(search_page_->preview_);

  // Add sort field texts
  for (int i=0 ; i<SearchTerm::FieldCount ; ++i) {
    const SearchTerm::Field field = SearchTerm::Field(i);
    const QString field_name = SearchTerm::FieldName(field);
    sort_ui_->field_value->addItem(field_name);
  }
  connect(sort_ui_->field_value, SIGNAL(currentIndexChanged(int)), SLOT(UpdateSortOrder()));
  UpdateSortOrder();

  // Set the sort and limit radio buttons back to their defaults - they would
  // have been changed by setupUi
  sort_ui_->random->setChecked(true);
  sort_ui_->limit_none->setChecked(true);

  // Set up the preview widget that's already at the bottom of the sort page
  sort_ui_->preview->set_library(library_);

  // Configure the page text
  search_page_->setTitle(tr("Search terms"));
  search_page_->setSubTitle(tr("A song will be included in the playlist if it matches these conditions."));
  sort_page->setTitle(tr("Search options"));
  sort_page->setSubTitle(tr("Choose how the playlist is sorted and how many songs it will contain."));
  sort_page->setFinalPage(true);

  // Add the pages
  const int first_page = wizard->addPage(search_page_);
  wizard->addPage(sort_page);
  return first_page;
}

GeneratorPtr QueryWizardPlugin::CreateGenerator() const {
  // TODO
  return GeneratorPtr();
}

void QueryWizardPlugin::UpdateSortOrder() {
  const SearchTerm::Field field =
      SearchTerm::Field(sort_ui_->field_value->currentIndex());
  const SearchTerm::Type type = SearchTerm::TypeOf(field);
  const QString asc = SearchTerm::FieldSortOrderText(type, true);
  const QString desc = SearchTerm::FieldSortOrderText(type, false);

  sort_ui_->order->clear();
  sort_ui_->order->addItem(asc);
  sort_ui_->order->addItem(desc);
}

void QueryWizardPlugin::AddSearchTerm() {
  SearchTermWidget* widget =
      new SearchTermWidget(library_, search_page_);
  connect(widget, SIGNAL(RemoveClicked()), SLOT(RemoveSearchTerm()));
  connect(widget, SIGNAL(Changed()), SLOT(UpdateTermPreview()));

  search_page_->layout_->insertWidget(search_page_->terms_.count(), widget);
  search_page_->terms_ << widget;

  UpdateTermPreview();
}

void QueryWizardPlugin::RemoveSearchTerm() {
  SearchTermWidget* widget =
      qobject_cast<SearchTermWidget*>(sender());
  if (!widget)
    return;

  const int index = search_page_->terms_.indexOf(widget);
  if (index == -1)
    return;

  delete search_page_->terms_.takeAt(index);
  UpdateTermPreview();
}

void QueryWizardPlugin::UpdateTermPreview() {
  Search search = MakeSearch();
  emit search_page_->completeChanged();
  if (!search.is_valid())
    return;

  // Don't apply limits in the term page
  search.limit_ = -1;

  search_page_->preview_->Update(search);
}

void QueryWizardPlugin::UpdateSortPreview() {
  Search search = MakeSearch();
  if (!search.is_valid())
    return;

  sort_ui_->preview->Update(search);
}

Search QueryWizardPlugin::MakeSearch() const {
  Search ret;

  // Search type
  ret.search_type_ = Search::SearchType(search_page_->ui_->type->currentIndex());

  // Search terms
  foreach (SearchTermWidget* widget, search_page_->terms_) {
    SearchTerm term = widget->Term();
    if (term.is_valid())
      ret.terms_ << term;
  }

  // Sort order
  if (sort_ui_->random->isChecked()) {
    ret.sort_type_ = Search::Sort_Random;
  } else {
    const bool ascending = sort_ui_->order->currentIndex() == 0;
    ret.sort_type_ = ascending ? Search::Sort_FieldAsc :
                                 Search::Sort_FieldDesc;
    ret.sort_field_ = SearchTerm::Field(sort_ui_->field_value->currentIndex());
  }

  // Limit
  if (sort_ui_->limit_none->isChecked())
    ret.limit_ = -1;
  else
    ret.limit_ = sort_ui_->limit_value->value();

  return ret;
}

void QueryWizardPlugin::SearchTypeChanged() {
  const bool all = search_page_->ui_->type->currentIndex() == 2;
  search_page_->ui_->terms_group->setEnabled(!all);

  UpdateTermPreview();
}


QueryWizardPlugin::SearchPage::SearchPage(QWidget* parent)
  : QWizardPage(parent),
    ui_(new Ui_SmartPlaylistQuerySearchPage)
{
  ui_->setupUi(this);
}

bool QueryWizardPlugin::SearchPage::isComplete() const {
  if (ui_->type->currentIndex() == 2) // All songs
    return true;

  foreach (SearchTermWidget* widget, terms_) {
    if (!widget->Term().is_valid())
      return false;
  }
  return true;
}

} // namespace smart_playlists
