import sqlite3

# Datenbank verbinden
daten_c = sqlite3.connect('daten.db')
daten = daten_c.cursor()


def update_querschnitte(seite):
    faktor = 1
    if seite == "L":
        faktor = -1

    sql = 'SELECT VST, STREIFENNR, BREITE, BISBREITE, VNK, NNK FROM DB001030 WHERE STREIFEN IN ("M", "' + seite +\
          '") ORDER BY VNK, NNK, VST, STREIFENNR'
    daten.execute(sql)
    linke = daten.fetchall()

    vst = -1
    vnk = None
    nnk = None
    streifennr = -1
    abstand_vst1 = 0
    abstand_bst1 = 0
    for quer in linke:
        if vnk != quer[4] or nnk != quer[5] or vst != quer[0] or streifennr > quer[1]:
            abstand_vst1 = 0
            abstand_bst1 = 0
            vst = quer[0]
            vnk = quer[4]
            nnk = quer[5]
        if quer[1] == 0:
            abstand_vst1 = faktor * round(quer[2] / 2)
            abstand_bst1 = faktor * round(quer[3] / 2)
        else:
            abstand_vst2 = abstand_vst1 + faktor * quer[2]
            abstand_bst2 = abstand_bst1 + faktor * quer[3]
            sql2 = 'UPDATE DB001030 SET ABSTAND_VST1 = ' + str(abstand_vst1) + ', ABSTAND_VST2 = ' + \
                   str(abstand_vst2) + ', ABSTAND_BST1 = ' + str(abstand_bst1) + ', ABSTAND_BST2 = ' + \
                   str(abstand_bst2) + ' WHERE VNK = "' + str(quer[4]) + '" AND NNK = "' + str(quer[5]) + \
                   '" AND VST = ' + str(quer[0]) + ' AND STREIFEN = "' + seite + '" AND STREIFENNR = ' + str(quer[1])
            daten.execute(sql2)

            abstand_vst1 = abstand_vst2
            abstand_bst1 = abstand_bst2
        streifennr = quer[1]


# Felder anlegen
try:
    daten.execute('''ALTER TABLE DB001030 ADD COLUMN ABSTAND_VST1 int default NULL''')
    daten.execute('''ALTER TABLE DB001030 ADD COLUMN ABSTAND_BST1 int default NULL''')
    daten.execute('''ALTER TABLE DB001030 ADD COLUMN ABSTAND_VST2 int default NULL''')
    daten.execute('''ALTER TABLE DB001030 ADD COLUMN ABSTAND_BST2 int default NULL''')
except sqlite3.OperationalError:
    print("Felder vorhanden")


# Mittelstreifen berechnen
print("Mitte")
daten.execute('UPDATE DB001030 SET ABSTAND_VST1 = ROUND(- BREITE / 2), ABSTAND_VST2 = ROUND(BREITE / 2), ' +
              'ABSTAND_BST1 = ROUND(- BISBREITE / 2), ABSTAND_BST2 = ROUND(BISBREITE / 2) WHERE STREIFEN = "M"')

print("Links")
update_querschnitte("L")

print("Rechts")
update_querschnitte("R")


daten_c.commit()

print("Fertig")
