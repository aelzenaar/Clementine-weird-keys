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

#ifndef SMARTPLAYLISTWIZARD_H
#define SMARTPLAYLISTWIZARD_H

#include <QWizard>

class LibraryBackend;

class QSignalMapper;

namespace smart_playlists {

class WizardPlugin;

class Wizard : public QWizard {
  Q_OBJECT

public:
  Wizard(LibraryBackend* library, QWidget* parent);
  ~Wizard();

private:
  struct TypePage : public QWizardPage {
    TypePage(QWidget* parent) : QWizardPage(parent), next_id_(0) {}
    int nextId() const { return next_id_; }
    int next_id_;
  };

  void AddPlugin(WizardPlugin* plugin);

private slots:
  void TypeChanged(int index);

private:
  LibraryBackend* library_;
  TypePage* type_page_;
  QSignalMapper* type_mapper_;

  QList<WizardPlugin*> plugins_;
};

} // namespace

#endif // SMARTPLAYLISTWIZARD_H
