#ifndef HTML_H
#define HTML_H

const char* HTML_ROOT = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW Web</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        .status {
            background: #1a1a1a;
            padding: 15px;
            border-radius: 8px;
            border: 1px solid #333;
            margin: 20px 0;
            white-space: pre-line;
        }
        .nav {
            text-align: center;
            margin: 20px 0;
        }
        .nav a {
            color: #4da6ff;
            text-decoration: none;
            margin: 0 15px;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .nav a:hover {
            background: #4da6ff;
            color: #101010;
        }
        .error {
            color: #ff4444;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h1>REW - Ventilacijska enota</h1>
    <div class="status">%s</div>
    <div class="nav">
        <a href="/history">Zgodovina</a>
        <a href="/logs">Logi</a>
        <a href="/help">Pomoč</a>
        <a href="/delete">Brisanje</a>
    </div>
</body>
</html>)rawliteral";

const char* HTML_HELP = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW - Pomoč</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
            max-width: 800px;
            margin: 20px auto;
        }
        h1 {
            color: white;
            text-align: center;
        }
        ul {
            line-height: 1.6;
        }
        li {
            margin: 10px 0;
        }
        a {
            color: #4da6ff;
        }
        .back {
            text-align: center;
            margin-top: 30px;
        }
        .back a {
            color: #4da6ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .back a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>Pomoč - REW Web vmesnik</h1>
    <ul>
        <li><strong>Pregled zgodovine:</strong> Na /history izberi from/to in type (sens/vent/all) za table senzorjev/ventilatorjev.</li>
        <li><strong>Izvoz:</strong> Uporabi /history/download?type=sens&from=...&to=... za CSV senzorjev (podobno za vent); za loge /logs/export?date=...&time=... .</li>
        <li><strong>Pregled logov:</strong> Na /logs izberi datum in uro za 1-urno okno logov.</li>
        <li><strong>Brisanje:</strong> Na /delete izberi do-datuma za brisanje starejših datotek (potrdi).</li>
        <li><strong>Live status:</strong> Na root strani vidi trenutne vrednosti senzorjev.</li>
    </ul>
    <div class="back">
        <a href="/">Nazaj na začetno stran</a>
    </div>
</body>
</html>)rawliteral";

const char* HTML_DELETE_FORM = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW - Brisanje</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        form {
            max-width: 400px;
            margin: 20px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
        }
        label {
            display: block;
            margin-bottom: 10px;
            font-weight: bold;
        }
        input[type="date"] {
            width: 100%;
            padding: 8px;
            margin-bottom: 15px;
            background: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            color: #e0e0e0;
        }
        input[type="submit"] {
            background: #ff4444;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
            width: 100%;
        }
        input[type="submit"]:hover {
            background: #cc3333;
        }
        .back {
            text-align: center;
            margin-top: 20px;
        }
        .back a {
            color: #4da6ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .back a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>Brisanje starih datotek</h1>
    <form method="GET" action="/delete">
        <label for="up_to">Pobriši vse starejše od datuma:</label>
        <input type="date" id="up_to" name="up_to" value="%s" required>
        <input type="submit" value="Brisanje">
    </form>
    <div class="back">
        <a href="/">Nazaj na začetno stran</a>
    </div>
</body>
</html>)rawliteral";

const char* HTML_DELETE_CONFIRM = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW - Potrditev brisanja</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        .confirm {
            max-width: 500px;
            margin: 20px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
            text-align: center;
        }
        .warning {
            color: #ffaa00;
            font-weight: bold;
            margin-bottom: 20px;
        }
        .buttons {
            display: flex;
            gap: 20px;
            justify-content: center;
        }
        .btn {
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            text-decoration: none;
            display: inline-block;
            min-width: 100px;
        }
        .btn-confirm {
            background: #ff4444;
            color: white;
        }
        .btn-confirm:hover {
            background: #cc3333;
        }
        .btn-cancel {
            background: #666;
            color: white;
        }
        .btn-cancel:hover {
            background: #555;
        }
    </style>
</head>
<body>
    <h1>Potrditev brisanja</h1>
    <div class="confirm">
        <div class="warning">OPOZORILO: Ta operacija bo izbrisala vse datoteke starejše od %s!</div>
        <p>To vključuje zgodovinske datoteke senzorjev, ventilatorjev in logov.</p>
        <div class="buttons">
            <a href="/delete?up_to=%s&confirm=yes" class="btn btn-confirm">Potrdi</a>
            <a href="/delete" class="btn btn-cancel">Prekliči</a>
        </div>
    </div>
</body>
</html>)rawliteral";

const char* HTML_LOGS_FORM = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW - Logi</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        form {
            max-width: 600px;
            margin: 20px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
        }
        .form-row {
            display: flex;
            gap: 20px;
            margin-bottom: 15px;
            align-items: center;
        }
        label {
            min-width: 80px;
        }
        input[type="date"], input[type="time"] {
            padding: 8px;
            background: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            color: #e0e0e0;
        }
        input[type="submit"] {
            background: #4da6ff;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background: #3a8ae6;
        }
        .content {
            margin-top: 20px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: #1a1a1a;
            border: 1px solid #333;
        }
        th, td {
            padding: 8px;
            text-align: left;
            border: 1px solid #333;
        }
        th {
            background: #2a2a2a;
            color: white;
        }
        .row-r {
            color: #ff4444;
        }
        .row-c {
            color: #44ff44;
        }
        .scrollable {
            overflow-y: auto;
            max-height: 80vh;
        }
        .warning {
            color: #ffaa00;
            font-style: italic;
            margin: 10px 0;
        }
        .back {
            text-align: center;
            margin-top: 20px;
        }
        .back a {
            color: #4da6ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .back a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>Logi sistema</h1>
    <form method="GET" action="/logs">
        <div class="form-row">
            <label for="date">Datum:</label>
            <input type="date" id="date" name="date" value="%s" required>
            <label for="time">Ura:</label>
            <input type="time" id="time" name="time" value="%s" required>
            <input type="submit" value="Prikaži">
        </div>
    </form>
    <div class="content">
        %s
    </div>
    <div class="back">
        <a href="/">Nazaj na začetno stran</a>
    </div>
</body>
</html>)rawliteral";

const char* HTML_HISTORY_FORM = R"rawliteral(
<!DOCTYPE html>
<html lang="sl">
<head>
    <meta charset="UTF-8">
    <title>REW - Zgodovina</title>
    <style>
        body {
            background: #101010;
            color: #e0e0e0;
            font-family: sans-serif;
            margin: 20px;
        }
        h1 {
            color: white;
            text-align: center;
        }
        form {
            max-width: 700px;
            margin: 20px auto;
            background: #1a1a1a;
            padding: 20px;
            border-radius: 8px;
            border: 1px solid #333;
        }
        .form-row {
            display: flex;
            gap: 20px;
            margin-bottom: 15px;
            align-items: center;
            flex-wrap: wrap;
        }
        label {
            min-width: 60px;
        }
        input[type="date"] {
            padding: 8px;
            background: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            color: #e0e0e0;
        }
        select {
            padding: 8px;
            background: #2a2a2a;
            border: 1px solid #555;
            border-radius: 4px;
            color: #e0e0e0;
        }
        input[type="submit"] {
            background: #4da6ff;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background: #3a8ae6;
        }
        .content {
            margin-top: 20px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background: #1a1a1a;
            border: 1px solid #333;
            margin-bottom: 20px;
        }
        th, td {
            padding: 6px;
            text-align: left;
            border: 1px solid #333;
            font-size: 12px;
        }
        th {
            background: #2a2a2a;
            color: white;
            font-weight: bold;
        }
        .scrollable {
            overflow-y: auto;
            max-height: 80vh;
        }
        .warning {
            color: #ffaa00;
            font-style: italic;
            margin: 10px 0;
        }
        .table-title {
            font-size: 16px;
            font-weight: bold;
            margin: 15px 0 5px 0;
            color: white;
        }
        .back {
            text-align: center;
            margin-top: 20px;
        }
        .back a {
            color: #4da6ff;
            text-decoration: none;
            padding: 10px 20px;
            border: 1px solid #4da6ff;
            border-radius: 5px;
        }
        .back a:hover {
            background: #4da6ff;
            color: #101010;
        }
    </style>
</head>
<body>
    <h1>Zgodovina podatkov</h1>
    <form method="GET" action="/history">
        <div class="form-row">
            <label for="from">Od:</label>
            <input type="date" id="from" name="from" value="%s" required>
            <label for="to">Do:</label>
            <input type="date" id="to" name="to" value="%s" required>
            <label for="type">Tip:</label>
            <select id="type" name="type">
                <option value="all">Vse</option>
                <option value="sens">Senzorji</option>
                <option value="vent">Ventilatorji</option>
            </select>
            <input type="submit" value="Prikaži">
        </div>
    </form>
    <div class="content">
        <div style="overflow-y: auto; height: 80vh;">%s</div>
    </div>
    <div class="back">
        <a href="/">Nazaj na začetno stran</a>
    </div>
</body>
</html>)rawliteral";

#endif
