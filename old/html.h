// html.h

#ifndef HTML_H
#define HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <meta charset="UTF-8">
    <title>CYD - Ventilacijski sistem</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 10px; font-size: 16px; }
        h1 { text-align: center; font-size: 20px; }
        .form-container { max-width: 500px; margin: auto; }
        table { width: 100%; border-collapse: collapse; font-size: 14px; }
        th, td { padding: 5px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #4CAF50; color: white; }
        input[type=number], input[type=checkbox], input[type=date] { padding: 3px; font-size: 14px; }
        input[type=number] { width: 60px; }
        input[type=date] { width: 120px; }
        .error { color: red; text-align: center; font-size: 14px; }
        .success { color: green; text-align: center; font-size: 14px; }
        .button-group { text-align: center; margin: 10px 0; }
        .submit-btn { padding: 5px 10px; margin-right: 5px; background-color: #4CAF50; color: white; border: none; cursor: pointer; font-size: 14px; }
        .reset-btn { background-color: #f44336; }
        .tab { display: flex; justify-content: center; border-bottom: 1px solid #ccc; margin-bottom: 10px; }
        .tab button { background-color: #f1f1f1; border: none; outline: none; cursor: pointer; padding: 10px 16px; font-size: 14px; margin: 0 5px; }
        .tab button:hover { background-color: #ddd; }
        .tab button.active { background-color: #4CAF50; color: white; }
        .tabcontent { display: none; }
        .tabcontent.active { display: block; }
        ul { list-style-type: none; padding: 0; }
        li { margin-bottom: 10px; }
        h3 { font-size: 18px; margin-bottom: 5px; }
        p { font-size: 14px; margin: 0; }
    </style>
</head>
<body>
    <h1>Ventilacijski sistem - Nastavitve</h1>
    <div class="tab">
        <button class="tablinks active" onclick="openTab(event, 'Settings')">Nastavitve</button>
        <button class="tablinks" onclick="openTab(event, 'History')">Zgodovina</button>
        <button class="tablinks" onclick="openTab(event, 'Help')">Pomoč</button>
    </div>
    <div id="Settings" class="tabcontent active">
        <div class="form-container">
            <form id="settingsForm">
                <div class="error" id="error-message"></div>
                <div class="success" id="success-message"></div>
                <div class="button-group">
                    <input type="button" value="Shrani" class="submit-btn" onclick="saveSettings()">
                    <input type="button" value="Ponastavi na privzeto" class="submit-btn reset-btn" onclick="resetSettings()">
                </div>
                <table>
                    <tr>
                        <th>Parameter</th>
                        <th>Vnos</th>
                    </tr>
                    <tr>
                        <td>Meja vlage Kopalnica/Utility (0–100 %)</td>
                        <td><input type="number" name="humThreshold" id="humThreshold" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Trajanje ventilatorjev (60–6000 s)</td>
                        <td><input type="number" name="fanDuration" id="fanDuration" step="1" min="60" max="6000"></td>
                    </tr>
                    <tr>
                        <td>Čakanje Utility/WC (60–6000 s)</td>
                        <td><input type="number" name="fanOffDuration" id="fanOffDuration" step="1" min="60" max="6000"></td>
                    </tr>
                    <tr>
                        <td>Čakanje Kopalnica (60–6000 s)</td>
                        <td><input type="number" name="fanOffDurationKop" id="fanOffDurationKop" step="1" min="60" max="6000"></td>
                    </tr>
                    <tr>
                        <td>Meja nizke zunanje temperature (-20–40 °C)</td>
                        <td><input type="number" name="tempLowThreshold" id="tempLowThreshold" step="1" min="-20" max="40"></td>
                    </tr>
                    <tr>
                        <td>Meja minimalne zunanje temperature (-20–40 °C)</td>
                        <td><input type="number" name="tempMinThreshold" id="tempMinThreshold" step="1" min="-20" max="40"></td>
                    </tr>
                    <tr>
                        <td>DND dovoli avtomatiko</td>
                        <td><input type="checkbox" name="dndAllowAutomatic" id="dndAllowAutomatic"></td>
                    </tr>
                    <tr>
                        <td>DND dovoli polavtomatiko</td>
                        <td><input type="checkbox" name="dndAllowSemiautomatic" id="dndAllowSemiautomatic"></td>
                    </tr>
                    <tr>
                        <td>DND dovoli ročno upravljanje</td>
                        <td><input type="checkbox" name="dndAllowManual" id="dndAllowManual"></td>
                    </tr>
                    <tr>
                        <td>Trajanje cikla Dnevni prostor (60–6000 s)</td>
                        <td><input type="number" name="cycleDurationDS" id="cycleDurationDS" step="1" min="60" max="6000"></td>
                    </tr>
                    <tr>
                        <td>Aktivni delež cikla Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="cycleActivePercentDS" id="cycleActivePercentDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Meja vlage Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="humThresholdDS" id="humThresholdDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Visoka meja vlage Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="humThresholdHighDS" id="humThresholdHighDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Ekstremno visoka vlaga Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="humExtremeHighDS" id="humExtremeHighDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Nizka meja CO2 Dnevni prostor (400–2000 ppm)</td>
                        <td><input type="number" name="co2ThresholdLowDS" id="co2ThresholdLowDS" step="1" min="400" max="2000"></td>
                    </tr>
                    <tr>
                        <td>Visoka meja CO2 Dnevni prostor (400–2000 ppm)</td>
                        <td><input type="number" name="co2ThresholdHighDS" id="co2ThresholdHighDS" step="1" min="400" max="2000"></td>
                    </tr>
                    <tr>
                        <td>Nizko povečanje Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="incrementPercentLowDS" id="incrementPercentLowDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Visoko povečanje Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="incrementPercentHighDS" id="incrementPercentHighDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Povečanje za temperaturo Dnevni prostor (0–100 %)</td>
                        <td><input type="number" name="incrementPercentTempDS" id="incrementPercentTempDS" step="1" min="0" max="100"></td>
                    </tr>
                    <tr>
                        <td>Idealna temperatura Dnevni prostor (-20–40 °C)</td>
                        <td><input type="number" name="tempIdealDS" id="tempIdealDS" step="1" min="-20" max="40"></td>
                    </tr>
                    <tr>
                        <td>Ekstremno visoka temperatura Dnevni prostor (-20–40 °C)</td>
                        <td><input type="number" name="tempExtremeHighDS" id="tempExtremeHighDS" step="1" min="-20" max="40"></td>
                    </tr>
                    <tr>
                        <td>Ekstremno nizka temperatura Dnevni prostor (-20–40 °C)</td>
                        <td><input type="number" name="tempExtremeLowDS" id="tempExtremeLowDS" step="1" min="-20" max="40"></td>
                    </tr>
                </table>
            </form>
        </div>
    </div>
    <div id="History" class="tabcontent">
        <div class="form-container">
            <div class="error" id="history-error-message"></div>
            <div class="success" id="history-success-message"></div>
            <div class="button-group">
                <input type="date" id="historyDate" min="2025-01-01" max="2025-07-11">
                <input type="button" value="Prenesi podatke" class="submit-btn" onclick="downloadHistory()">
            </div>
        </div>
    </div>
    <div id="Help" class="tabcontent">
        <div class="form-container">
            <ul>
                <li>
                    <h3>HUM_THRESHOLD</h3>
                    <p>Meja vlage za avtomatsko aktivacijo ventilatorjev v Kopalnici in Utility (0–100 %).</p>
                </li>
                <li>
                    <h3>FAN_DURATION</h3>
                    <p>Trajanje delovanja ventilatorjev po sprožilcu (60–6000 s).</p>
                </li>
                <li>
                    <h3>FAN_OFF_DURATION</h3>
                    <p>Čas čakanja pred naslednjim ciklom v Utility in WC (60–6000 s).</p>
                </li>
                <li>
                    <h3>FAN_OFF_DURATION_KOP</h3>
                    <p>Čas čakanja pred naslednjim ciklom v Kopalnici (60–6000 s).</p>
                </li>
                <li>
                    <h3>TEMP_LOW_THRESHOLD</h3>
                    <p>Zunanja temperatura, pod katero se delovanje ventilatorjev zmanjša (-20–40 °C).</p>
                </li>
                <li>
                    <h3>TEMP_MIN_THRESHOLD</h3>
                    <p>Zunanja temperatura, pod katero se ventilatorji ustavijo (-20–40 °C).</p>
                </li>
                <li>
                    <h3>DND_ALLOW_AUTOMATIC</h3>
                    <p>Dovoli avtomatsko aktivacijo ventilatorjev med DND (nočni čas).</p>
                </li>
                <li>
                    <h3>DND_ALLOW_SEMIAUTOMATIC</h3>
                    <p>Dovoli polavtomatske sprožilce (npr. luči) med DND.</p>
                </li>
                <li>
                    <h3>DND_ALLOW_MANUAL</h3>
                    <p>Dovoli ročne sprožilce med DND.</p>
                </li>
                <li>
                    <h3>CYCLE_DURATION_DS</h3>
                    <p>Trajanje cikla prezračevanja v Dnevnem prostoru (60–6000 s).</p>
                </li>
                <li>
                    <h3>CYCLE_ACTIVE_PERCENT_DS</h3>
                    <p>Aktivni delež cikla v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>HUM_THRESHOLD_DS</h3>
                    <p>Meja vlage za povečanje cikla v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>HUM_THRESHOLD_HIGH_DS</h3>
                    <p>Visoka meja vlage za večje povečanje cikla v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>HUM_EXTREME_HIGH_DS</h3>
                    <p>Ekstremno visoka zunanja vlaga za zmanjšanje cikla (0–100 %).</p>
                </li>
                <li>
                    <h3>CO2_THRESHOLD_LOW_DS</h3>
                    <p>Nizka meja CO2 za povečanje cikla v Dnevnem prostoru (400–2000 ppm).</p>
                </li>
                <li>
                    <h3>CO2_THRESHOLD_HIGH_DS</h3>
                    <p>Visoka meja CO2 za večje povečanje cikla v Dnevnem prostoru (400–2000 ppm).</p>
                </li>
                <li>
                    <h3>INCREMENT_PERCENT_LOW_DS</h3>
                    <p>Nizko povečanje cikla ob mejah vlage/CO2 v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>INCREMENT_PERCENT_HIGH_DS</h3>
                    <p>Visoko povečanje cikla ob visokih mejah vlage/CO2 v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>INCREMENT_PERCENT_TEMP_DS</h3>
                    <p>Povečanje cikla za hlajenje/ogrevanje v Dnevnem prostoru (0–100 %).</p>
                </li>
                <li>
                    <h3>TEMP_IDEAL_DS</h3>
                    <p>Idealna temperatura v Dnevnem prostoru (-20–40 °C).</p>
                </li>
                <li>
                    <h3>TEMP_EXTREME_HIGH_DS</h3>
                    <p>Ekstremno visoka zunanja temperatura za zmanjšanje cikla (-20–40 °C).</p>
                </li>
                <li>
                    <h3>TEMP_EXTREME_LOW_DS</h3>
                    <p>Ekstremno nizka zunanja temperatura za zmanjšanje cikla (-20–40 °C).</p>
                </li>
            </ul>
        </div>
    </div>
    <script>
        function openTab(evt, tabName) {
            var i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("tabcontent");
            for (i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none";
            }
            tablinks = document.getElementsByClassName("tablinks");
            for (i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            evt.currentTarget.className += " active";
        }

        let updateInterval;

        function updateSettings() {
            fetch("/data")
                .then(response => response.json())
                .then(data => {
                    document.getElementById("humThreshold").value = data.HUM_THRESHOLD;
                    document.getElementById("fanDuration").value = data.FAN_DURATION;
                    document.getElementById("fanOffDuration").value = data.FAN_OFF_DURATION;
                    document.getElementById("fanOffDurationKop").value = data.FAN_OFF_DURATION_KOP;
                    document.getElementById("tempLowThreshold").value = data.TEMP_LOW_THRESHOLD;
                    document.getElementById("tempMinThreshold").value = data.TEMP_MIN_THRESHOLD;
                    document.getElementById("dndAllowAutomatic").checked = data.DND_ALLOW_AUTOMATIC;
                    document.getElementById("dndAllowSemiautomatic").checked = data.DND_ALLOW_SEMIAUTOMATIC;
                    document.getElementById("dndAllowManual").checked = data.DND_ALLOW_MANUAL;
                    document.getElementById("cycleDurationDS").value = data.CYCLE_DURATION_DS;
                    document.getElementById("cycleActivePercentDS").value = data.CYCLE_ACTIVE_PERCENT_DS;
                    document.getElementById("humThresholdDS").value = data.HUM_THRESHOLD_DS;
                    document.getElementById("humThresholdHighDS").value = data.HUM_THRESHOLD_HIGH_DS;
                    document.getElementById("humExtremeHighDS").value = data.HUM_EXTREME_HIGH_DS;
                    document.getElementById("co2ThresholdLowDS").value = data.CO2_THRESHOLD_LOW_DS;
                    document.getElementById("co2ThresholdHighDS").value = data.CO2_THRESHOLD_HIGH_DS;
                    document.getElementById("incrementPercentLowDS").value = data.INCREMENT_PERCENT_LOW_DS;
                    document.getElementById("incrementPercentHighDS").value = data.INCREMENT_PERCENT_HIGH_DS;
                    document.getElementById("incrementPercentTempDS").value = data.INCREMENT_PERCENT_TEMP_DS;
                    document.getElementById("tempIdealDS").value = data.TEMP_IDEAL_DS;
                    document.getElementById("tempExtremeHighDS").value = data.TEMP_EXTREME_HIGH_DS;
                    document.getElementById("tempExtremeLowDS").value = data.TEMP_EXTREME_LOW_DS;
                })
                .catch(error => console.error('Napaka pri pridobivanju nastavitev:', error));
        }

        function saveSettings() {
            var form = document.getElementById("settingsForm");
            var formData = new FormData(form);
            document.getElementById("error-message").textContent = "";
            document.getElementById("success-message").textContent = "";
            fetch("/settings/update", { method: "POST", body: formData })
                .then(response => {
                    if (!response.ok) {
                        throw new Error("Napaka pri shranjevanju: " + response.statusText);
                    }
                    return response.text();
                })
                .then(data => {
                    if (data !== "Zahtevek poslan, čakanje na potrditev CE") {
                        document.getElementById("error-message").textContent = data;
                        return;
                    }
                    document.getElementById("success-message").textContent = data;
                    let checkStatus = setInterval(() => {
                        fetch("/settings/status")
                            .then(response => {
                                if (!response.ok) {
                                    throw new Error("Napaka pri preverjanju statusa: " + response.statusText);
                                }
                                return response.json();
                            })
                            .then(status => {
                                if (!status.waiting) {
                                    clearInterval(checkStatus);
                                    if (status.success) {
                                        document.getElementById("success-message").textContent = status.message;
                                    } else {
                                        document.getElementById("error-message").textContent = status.message;
                                    }
                                }
                            })
                            .catch(error => {
                                clearInterval(checkStatus);
                                document.getElementById("error-message").textContent = "Napaka pri preverjanju statusa: " + error;
                                document.getElementById("success-message").textContent = "";
                            });
                    }, 1000);
                })
                .catch(error => {
                    document.getElementById("error-message").textContent = error.message;
                    document.getElementById("success-message").textContent = "";
                });
        }

        function resetSettings() {
            updateSettings();
        }

        function downloadHistory() {
            var historyDate = document.getElementById("historyDate").value;
            if (!historyDate) {
                document.getElementById("history-error-message").textContent = "Izberite datum";
                return;
            }
            var yyyymmdd = historyDate.replace(/-/g, "");
            var year = parseInt(yyyymmdd.substring(0, 4));
            var month = parseInt(yyyymmdd.substring(4, 6));
            var day = parseInt(yyyymmdd.substring(6, 8));
            if (year !== 2025 || month < 1 || month > 12 || day < 1 || day > 31) {
                document.getElementById("history-error-message").textContent = "Neveljaven datum (2025-01-01 do 2025-07-11)";
                return;
            }
            document.getElementById("history-error-message").textContent = "";
            document.getElementById("history-success-message").textContent = "";
            var formData = new FormData();
            formData.append("historyDate", yyyymmdd);
            fetch("/history", { method: "POST", body: formData })
                .then(response => {
                    if (!response.ok) {
                        throw new Error("Napaka pri zahtevku: " + response.statusText);
                    }
                    return response.text();
                })
                .then(data => {
                    if (data !== "Zahtevek poslan, čakanje na potrditev CE") {
                        document.getElementById("history-error-message").textContent = data;
                        return;
                    }
                    document.getElementById("history-success-message").textContent = data;
                    let checkStatus = setInterval(() => {
                        fetch("/settings/status")
                            .then(response => {
                                if (!response.ok) {
                                    throw new Error("Napaka pri preverjanju statusa: " + response.statusText);
                                }
                                return response.json();
                            })
                            .then(status => {
                                if (!status.waiting) {
                                    clearInterval(checkStatus);
                                    if (status.success) {
                                        document.getElementById("history-success-message").textContent = status.message;
                                        fetch("/history/data")
                                            .then(response => {
                                                if (!response.ok) {
                                                    throw new Error("Napaka pri prenosu podatkov: " + response.statusText);
                                                }
                                                return response.blob();
                                            })
                                            .then(blob => {
                                                const url = window.URL.createObjectURL(blob);
                                                const a = document.createElement("a");
                                                a.href = url;
                                                a.download = yyyymmdd + ".zip";
                                                document.body.appendChild(a);
                                                a.click();
                                                a.remove();
                                                window.URL.revokeObjectURL(url);
                                            })
                                            .catch(error => {
                                                document.getElementById("history-error-message").textContent = error.message;
                                                document.getElementById("history-success-message").textContent = "";
                                            });
                                    } else {
                                        document.getElementById("history-error-message").textContent = status.message;
                                        document.getElementById("history-success-message").textContent = "";
                                    }
                                }
                            })
                            .catch(error => {
                                clearInterval(checkStatus);
                                document.getElementById("history-error-message").textContent = "Napaka pri preverjanju statusa: " + error;
                                document.getElementById("history-success-message").textContent = "";
                            });
                    }, 1000);
                })
                .catch(error => {
                    document.getElementById("history-error-message").textContent = error.message;
                    document.getElementById("history-success-message").textContent = "";
                });
        }

        updateInterval = setInterval(updateSettings, 10000);
        updateSettings();

        // Ustavljanje updateInterval ob fokusiranju na inpute
        const inputs = document.querySelectorAll('#settingsForm input');
        inputs.forEach(input => {
            input.addEventListener('focus', () => clearInterval(updateInterval));
            input.addEventListener('blur', () => updateInterval = setInterval(updateSettings, 10000));
        });
    </script>
</body>
</html>
)rawliteral";

#endif // HTML_H
// html.h