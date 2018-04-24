import sqlite3
import math


# Normalisiert Vektoren
def norm(p_dx, p_dy):
    p_s = math.sqrt(p_dx * p_dx + p_dy * p_dy)
    if p_s > 0:
        p_dx = p_dx / p_s
        p_dy = p_dy / p_s
    return p_dx, p_dy, p_s


def write_linestring(p_punkte):
    text = "LINESTRING ("
    for p_p in p_punkte:
        text += str(p_p[1]) + " " + str(p_p[2]) + ", "
    if len(p_punkte) > 0:
        text = text[:-2]
    text += "); " + str(row[0]) + "; " + str(row[1])
    # print(text)
    txt.write(text + "\n")


# WKT-Dateien anlegen
txt = open("querschnitte.txt", "w")
txt.write("Geometrie; VNK; NNK; VST; BST; STREIFEN; STREIFENNR; ABSTAND_VST1; " +
          "ABSTAND_VST2; ABSTAND_BST1; ABSTAND_BST2; ART\n")

txt2 = open("achsen.txt", "w")
txt2.write("Geometrie; VNK; NNK\n")


# Datenbankverbindung herstellen
daten_c = sqlite3.connect('daten.db')
daten = daten_c.cursor()

# Abschnitte selektieren und durchgehen
daten.execute('SELECT VNK, NNK, LEN FROM DBABSCHN')
abschn = daten.fetchall()


# Polygon schreiben
def write_polygon(p_x, p_y):
    text = "POLYGON (("
    for i in range(len(p_x)):
        text += str(p_x[i]) + " " + str(p_y[i]) + ", "
    if len(p_x) > 0:
        text = text[:-2]
    text += ")); " + str(row[0]) + "; " + str(row[1]) + "; " + str(quer[0]) + "; " + str(quer[1]) + "; " + \
            str(quer[2]) + "; " + str(quer[3]) + "; " + str(quer[4]) + "; " + str(quer[5]) + "; " + \
            str(quer[6]) + "; " + str(quer[7]) + "; " + str(quer[8])
    # print(text)
    txt.write(text + "\n")


for row in abschn:
    # Koordinaten der Achse abfragen
    sql = 'SELECT STATION, XKOORD, YKOORD, DX, DY FROM DB000255 WHERE VNK = "' + row[0] + '" AND NNK = "' + row[
        1] + '" ORDER BY SORT'
    daten.execute(sql)
    punkte = daten.fetchall()

    # Achse als WKT ablegen
    write_linestring(punkte)

    # Querschnitte laden
    sql = 'SELECT VST, BST, STREIFEN, STREIFENNR, ABSTAND_VST1, ABSTAND_VST2, ABSTAND_BST1, ABSTAND_BST2, ' + \
          'ART FROM DB001030 WHERE VNK = "' + row[0] + '" AND NNK = "' + row[1] + '" ORDER BY VST, STREIFEN, STREIFENNR'
    daten.execute(sql)

    for quer in daten.fetchall():
        # print(quer)

        x = []
        y = []

        c = 0

        pa = None
        for p in punkte:
            if p[0] >= quer[0] and c == 0 and pa is not None:
                # Berechnung Anfangspunkt
                dx = p[1] - pa[1]
                dy = p[2] - pa[2]
                diff = p[0] - pa[0]
                f = 0
                if diff > 0:
                    f = (quer[0] - pa[0]) / diff
                # print(f)

                dxn, dyn, s = norm(dx, dy)
                # print("P1")
                # print(quer[4])
                x.append(pa[1] + dx * f + dyn * quer[4] / 100)
                y.append(pa[2] + dy * f - dxn * quer[4] / 100)

                x.append(pa[1] + dx * f + dyn * quer[5] / 100)
                y.append(pa[2] + dy * f - dxn * quer[5] / 100)

                c = 1
            if c == 1 and p[0] <= quer[1]:
                # print("P2")
                # Prozentualer Abstand
                f = (p[0]-quer[0])/(quer[1]-quer[0])
                # print(f)

                # Abstand interpolieren
                a = quer[4]+f*(quer[6]-quer[4])
                # print(a)

                # Abstand 2 interpolieren
                b = quer[5]+f*(quer[7]-quer[5])
                # print(b)
                try:
                    x.insert(0, p[1] - p[4] * a / 100)
                    y.insert(0, p[2] + p[3] * a / 100)
                    x.append(p[1] - p[4] * b / 100)
                    y.append(p[2] + p[3] * b / 100)
                except TypeError:
                    break

            if c == 1 and p[0] > quer[1]:
                # print("P3")
                # Berechnung Endpunkt
                dx = p[1] - pa[1]
                dy = p[2] - pa[2]

                f = (quer[1] - pa[0]) / (p[0] - pa[0])
                # print(p[0])
                # print(f)

                dxn, dyn, s = norm(dx, dy)

                x.insert(0, pa[1] + dx * f + dyn * quer[6] / 100)
                y.insert(0, pa[2] + dy * f - dxn * quer[6] / 100)

                x.append(pa[1] + dx * f + dyn * quer[7] / 100)
                y.append(pa[2] + dy * f - dxn * quer[7] / 100)

                break

            pa = p

        # Polygon schließen
        if len(x) > 0:
            x.append(x[0])
            y.append(y[0])

        write_polygon(x, y)

txt.close()
