import sqlite3
import math

daten_c = sqlite3.connect('daten.db')

daten = daten_c.cursor()


# Vektor berechnen
def dist(p_x1, p_x2, p_y1, p_y2):
    p_dx = p_x2 - p_x1
    p_dy = p_y2 - p_y1
    return norm(p_dx, p_dy)


# Einheitsvektor berechnen
def norm(p_dx, p_dy):
    p_strecke = math.sqrt(p_dx * p_dx + p_dy * p_dy)
    if p_strecke > 0:
        p_dx = p_dx / p_strecke
        p_dy = p_dy / p_strecke
    return p_dx, p_dy, p_strecke


# Felder anlegen
try:
    daten.execute('''ALTER TABLE DB000255 ADD COLUMN STATION real default NULL''')
    daten.execute('''ALTER TABLE DB000255 ADD COLUMN ABSTAND real default NULL''')
    daten.execute('''ALTER TABLE DB000255 ADD COLUMN DX real default NULL''')
    daten.execute('''ALTER TABLE DB000255 ADD COLUMN DY real default NULL''')
except sqlite3.OperationalError:
    print("Felder vorhanden")


daten.execute('SELECT VNK, NNK, LEN FROM DBABSCHN')
abschn = daten.fetchall()
for row in abschn:
    x = 0
    y = 0
    dx = 0
    dy = 0
    dx_alt = 0
    dy_alt = 0
    i: int = 0
    laenge = 0

    sql = 'SELECT SORT, XKOORD, YKOORD FROM DB000255 WHERE VNK = "' + row[0] + '" AND NNK = "'+row[1]+'" ORDER BY SORT'
    daten.execute(sql)
    for punkt in daten.fetchall():
        i = punkt[0]
        if x != 0:
            # print(dist(z[3], x, z[4], y))
            dx_alt = dx
            dy_alt = dy
            dx, dy, strecke = dist(punkt[1], x, punkt[2], y)
            laenge += strecke
            if dx == 0:
                daten.execute(
                    'UPDATE DB000255 SET DX = ' + str(dx) + ', DY = ' + str(dy) + ' WHERE  VNK = "' + row[
                        0] + '" AND NNK = "' + row[1] + '" AND SORT = 0')

        if dx != 0:
            dx_alt += dx
            dy_alt += dy
            dx_alt, dy_alt, strecke_alt = norm(dx_alt, dy_alt)
            daten.execute(
                'UPDATE DB000255 SET DX = ' + str(dx_alt) + ', DY = ' + str(dy_alt) + ' WHERE  VNK = "' + row[
                    0] + '" AND NNK = "' + row[1] + '" AND SORT = ' + str(i - 1))

        x = punkt[1]
        y = punkt[2]

        daten.execute('UPDATE DB000255 SET ABSTAND = ' + str(laenge) + ' WHERE  VNK = "' + row[0] + '" AND NNK = "'
                      + row[1] + '" AND SORT = ' + str(i))
    daten.execute('UPDATE DB000255 SET DX = ' + str(dx) + ', DY = ' + str(dy) + ' WHERE  VNK = "' + row[0] +
                  '" AND NNK = "' + row[1] + '" AND SORT = ' + str(i))
    daten_c.commit()

    faktor = laenge / row[2]
    # print(faktor)

    daten.execute('UPDATE DB000255 SET STATION = (ABSTAND * ' + str(faktor) + ') WHERE  VNK = "' + row[0] +
                  '" AND NNK = "' + row[1] + '"')

    daten.execute('UPDATE DB000255 SET DX = ' + str(dx) + ', DY = ' + str(dy) + ', STATION = ' + str(row[2]) +
                  ' WHERE  VNK = "' + row[0] + '" AND NNK = "' + row[1] + '" AND SORT = ' + str(id))
    daten.execute('UPDATE DB000255 SET STATION = ' + str(row[2]) + ' WHERE  VNK = "' + row[0] + '" AND NNK = "' +
                  row[1] + '" AND STATION > ' + str(row[2]))

daten_c.commit()
daten_c.close()
