//============================================================================
// Name        : quer2geo.cpp
// Author      : Florian Timm
// Version     :
// Copyright   : MIT Licence
// Description : Querschnitte aus der TTSIB zu Geometrien
//============================================================================

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include "sqlite3.h"
using namespace std;

sqlite3 *db;

void einheit(double *dx, double s) {
	if (s > 0) {
		*dx = *dx / s;
	} else {
		*dx = 0;
	}
}

double strecke(double dx, double dy) {
	return sqrt(dx * dx + dy * dy);
}

double strecke(double x1, double x2, double y1, double y2) {
	return strecke(x2 - x1, y2 - y1);
}

void preparePoints() {
	sqlite3_stmt * stmt_update255;
	string sql_update255 =
			"UPDATE DB000255 SET DX = ?, DY = ? WHERE VNK = ? AND NNK = ? AND SORT = ?;";
	sqlite3_prepare_v2(db, sql_update255.c_str(), -1, &stmt_update255, NULL);

	sqlite3_stmt * stmt_update255a;
	string sql_update255a =
			"UPDATE DB000255 SET DX = ?, DY = ?, ABSTAND = ? WHERE VNK = ? AND NNK = ? AND SORT = ?;";
	sqlite3_prepare_v2(db, sql_update255a.c_str(), -1, &stmt_update255a, NULL);

	sqlite3_stmt * stmt_abstand;
	string sql_abstand =
			"UPDATE DB000255 SET ABSTAND = ? WHERE VNK = ? AND NNK = ? AND SORT = ?;";
	sqlite3_prepare_v2(db, sql_abstand.c_str(), -1, &stmt_abstand, NULL);

	sqlite3_stmt * stmt_station;
	string sql_station =
			"UPDATE DB000255 SET STATION = (ABSTAND * ?) WHERE VNK = ? AND NNK = ?;";
	sqlite3_prepare_v2(db, sql_station.c_str(), -1, &stmt_station, NULL);

	sqlite3_stmt * stmt_zugross;
	string sql_zugross =
			"UPDATE DB000255 SET STATION = ? WHERE  VNK = ? AND NNK = ? AND STATION > ?;";
	sqlite3_prepare_v2(db, sql_zugross.c_str(), -1, &stmt_zugross, NULL);

	string sql =
			"SELECT SORT, XKOORD, YKOORD FROM DB000255 WHERE VNK = ? AND NNK = ? ORDER BY SORT;";
	sqlite3_stmt * pps_punkte;
	sqlite3_prepare_v2(db, sql.c_str(), -1, &pps_punkte, NULL);

	sqlite3_exec(db,
			"ALTER TABLE DB000255 ADD COLUMN STATION real default NULL, ABSTAND real default NULL, DX real default NULL, DY real default NULL",
			0, 0, 0);
	sqlite3_stmt * stmt;
	string sql_abschnitte = "SELECT VNK, NNK, LEN FROM DBABSCHN";
	sqlite3_prepare_v2(db, sql_abschnitte.c_str(), -1, &stmt,
	NULL);
	int rc;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		char * vnk = (char *) sqlite3_column_text(stmt, 0);
		char * nnk = (char *) sqlite3_column_text(stmt, 1);
		int len = sqlite3_column_int(stmt, 2);
		printf("%s\t%s\n", vnk, nnk);

		sqlite3_bind_text(pps_punkte, 1, vnk, 10, 0);
		sqlite3_bind_text(pps_punkte, 2, nnk, 10, 0);

		sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

		double laenge = 0;
		double x_alt = 0, y_alt = 0;
		double dx = 0, dy = 0;
		double dx_alt = 0, dy_alt = 0;
		int i = 0;

		while ((rc = sqlite3_step(pps_punkte)) == SQLITE_ROW) {
			i = sqlite3_column_int(pps_punkte, 0);
			double x = sqlite3_column_double(pps_punkte, 1);
			double y = sqlite3_column_double(pps_punkte, 2);

			//printf("%f\t%f\n", x, y);

			if (x_alt != 0) {
				dx_alt = dx;
				dy_alt = dy;
				dx = x - x_alt;
				dy = y - y_alt;

				double s = strecke(dx, dy);
				laenge += s;

				einheit(&dx, s);
				einheit(&dy, s);

				if (dx == 0) {
					double dx1 = 0, dy1 = 0;
					if (s > 0) {
						dx1 = dx / s;
						dy1 = dy / s;
					}

					// dx und dy beim ersten Punkt setzen
					sqlite3_bind_double(stmt_update255, 1, dx1);
					sqlite3_bind_double(stmt_update255, 2, dy1);
					sqlite3_bind_text(stmt_update255, 3, vnk, 10, 0);
					sqlite3_bind_text(stmt_update255, 4, nnk, 10, 0);
					sqlite3_bind_int(stmt_update255, 5, 0);
					sqlite3_step(stmt_update255);
					sqlite3_reset(stmt_update255);
				}

			}

			if (dx != 0) {
				dx_alt += dx;
				dy_alt += dy;

				double s = strecke(dx_alt, dy_alt);
				einheit(&dx_alt, s);
				einheit(&dy_alt, s);

				// dx und dy beim vorherigen Punkt setzen (außer ersten)
				sqlite3_bind_double(stmt_update255, 1, dx_alt);
				sqlite3_bind_double(stmt_update255, 2, dy_alt);
				sqlite3_bind_text(stmt_update255, 3, vnk, 10, 0);
				sqlite3_bind_text(stmt_update255, 4, nnk, 10, 0);
				sqlite3_bind_int(stmt_update255, 5, i - 1);
				sqlite3_step(stmt_update255);
				sqlite3_reset(stmt_update255);
			}

			x_alt = x;
			y_alt = y;

			// Abstand setzen
			sqlite3_bind_double(stmt_abstand, 1, laenge);
			sqlite3_bind_text(stmt_abstand, 2, vnk, 10, 0);
			sqlite3_bind_text(stmt_abstand, 3, nnk, 10, 0);
			sqlite3_step(stmt_abstand);
			sqlite3_reset(stmt_abstand);

		}
		sqlite3_reset(pps_punkte);

		double faktor = laenge / len;
		sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);

		// Station berechnen
		sqlite3_bind_double(stmt_station, 1, faktor);
		sqlite3_bind_text(stmt_station, 2, vnk, 10, 0);
		sqlite3_bind_text(stmt_station, 3, nnk, 10, 0);
		sqlite3_step(stmt_station);
		sqlite3_reset(stmt_station);

		// dx, dy und Station beim letzen Punkt setzen
		sqlite3_bind_double(stmt_update255a, 1, dx);
		sqlite3_bind_double(stmt_update255a, 2, dy);
		sqlite3_bind_double(stmt_update255a, 3, len);
		sqlite3_bind_text(stmt_update255a, 4, vnk, 10, 0);
		sqlite3_bind_text(stmt_update255a, 5, nnk, 10, 0);
		sqlite3_bind_int(stmt_update255a, 6, i);
		sqlite3_step(stmt_update255a);
		sqlite3_reset(stmt_update255a);

		// zu große Stationen entfernen
		sqlite3_bind_double(stmt_zugross, 1, faktor);
		sqlite3_bind_text(stmt_zugross, 2, vnk, 10, 0);
		sqlite3_bind_text(stmt_zugross, 3, nnk, 10, 0);
		sqlite3_step(stmt_zugross);
		sqlite3_reset(stmt_zugross);

	}
}

void updateQuerschnitte(char* seite) {
	int faktor = 1;
	if (string("L").compare(seite)) {
		faktor = -1;
	}

	string sql =
			"SELECT VST, STREIFENNR, BREITE, BISBREITE, VNK, NNK FROM DB001030 WHERE STREIFEN IN (\"M\", ?) ORDER BY VNK, NNK, VST, STREIFENNR;";
	sqlite3_stmt * stmt;
	sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);

	string sql2 =
			"UPDATE DB001030 SET ABSTAND_VST1 = ?, ABSTAND_VST2 = ?, ABSTAND_BST1 = ?, ABSTAND_BST2 = ?"
					" WHERE VNK = ? AND NNK = ? AND VST = ? AND STREIFEN = ? AND STREIFENNR = ?;";
	sqlite3_stmt * stmt2;
	sqlite3_prepare_v2(db, sql2.c_str(), -1, &stmt2, NULL);

	sqlite3_bind_text(stmt, 1, seite, 1, 0);

	int vst = -1;
	string vnk = "VNK";
	string nnk = "NNK";
	int streifennr = -1;
	int abstand_vst1 = 0;
	int abstand_bst1 = 0;

	int rc;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

		if (vnk.compare((char *) sqlite3_column_text(stmt, 4)) != 0
				or nnk.compare((char*) sqlite3_column_text(stmt, 5)) != 0
				or vst != sqlite3_column_int(stmt, 0)
				or streifennr > sqlite3_column_int(stmt, 1)) {
			abstand_vst1 = 0;
			abstand_bst1 = 0;
			vst = sqlite3_column_int(stmt, 0);
			vnk = (char *) sqlite3_column_text(stmt, 4);
			nnk = (char *) sqlite3_column_text(stmt, 5);
		}

		if (sqlite3_column_int(stmt, 1) == 0) {
			abstand_vst1 = faktor * round(sqlite3_column_int(stmt, 2) / 2);
			abstand_bst1 = faktor * round(sqlite3_column_int(stmt, 3) / 2);
		} else {
			double abstand_vst2 = abstand_vst1
					+ faktor * sqlite3_column_int(stmt, 2);
			double abstand_bst2 = abstand_bst1
					+ faktor * sqlite3_column_int(stmt, 3);

			sqlite3_bind_double(stmt2, 1, abstand_vst1);
			sqlite3_bind_double(stmt2, 2, abstand_vst2);
			sqlite3_bind_double(stmt2, 3, abstand_bst1);
			sqlite3_bind_double(stmt2, 4, abstand_bst2);
			sqlite3_bind_text(stmt2, 5, vnk.c_str(), 10, 0);
			sqlite3_bind_text(stmt2, 6, nnk.c_str(), 10, 0);
			sqlite3_bind_int(stmt2, 7, vst);
			sqlite3_bind_text(stmt2, 8, (char *) seite, 1, 0);
			sqlite3_bind_int(stmt2, 9, sqlite3_column_int(stmt, 1));
			sqlite3_step(stmt2);
			sqlite3_reset(stmt2);

			abstand_vst1 = abstand_vst2;
			abstand_bst1 = abstand_bst2;
		}
		streifennr = sqlite3_column_int(stmt, 1);

	}
	sqlite3_reset(stmt);

}

void sumQuer() {

	fprintf(stdout, "Felder anlegen\n");
	sqlite3_exec(db,
			"ALTER TABLE DB001030 ADD COLUMN ABSTAND_VST1 int default NULL, ABSTAND_BST1 int default NULL, ABSTAND_VST2 int default NULL, ABSTAND_BST2 int default NULL",
			0, 0, 0);
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

	fprintf(stdout, "Mitte\n");
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	sqlite3_exec(db,
			"UPDATE DB001030 SET ABSTAND_VST1 = ROUND(- BREITE / 2), ABSTAND_VST2 = ROUND(BREITE / 2), ABSTAND_BST1 = ROUND(- BISBREITE / 2), ABSTAND_BST2 = ROUND(BISBREITE / 2) WHERE STREIFEN = \"M\"",
			0, 0, 0);
	sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);

	fprintf(stdout, "Links\n");
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	char * seite = "L";
	updateQuerschnitte(seite);
	sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);

	fprintf(stdout, "Rechts\n");
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	seite = "R";
	updateQuerschnitte(seite);
	sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
	fprintf(stdout, "Fertig!\n");
}

int main(int argc, char** argv) {
	fprintf(stdout, "Hallo\n");
	sqlite3_open("D:\\Querschnitt2Geo\\daten.db", &db);
	sqlite3_exec(db, "PRAGMA schema.synchronous = 0", 0, 0, 0);

	int auswahl;
	cout << "Prozess auswählen:" << endl;
	cout << "1: preparePoints" << endl;
	cout << "2: sumQuer" << endl;
	cin >> auswahl;

	switch (auswahl) {
	case 1:
		cout << "preparePoints gewählt:" << endl;
		preparePoints();
		break;
	case 2:
		cout << "sumQuer gewählt:" << endl;
		sumQuer();
		break;
	}

	sqlite3_close(db);

	return 0;
}

