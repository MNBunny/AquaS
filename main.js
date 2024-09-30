document.addEventListener("DOMContentLoaded", function () {
  const menuIcon = document.querySelector('.menu-icon');
  const sidebar = document.getElementById('sidebar');
  const closeButton = document.querySelector('.close-button');

  menuIcon.addEventListener('click', function () {
    sidebar.classList.toggle('sidebar-responsive');
  });

  closeButton.addEventListener('click', function () {
    sidebar.classList.remove('sidebar-responsive'); // Close the sidebar
  });
});

// Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBdUTGzi9iQ3asge53BP3UfLALtBghNggQ",
  authDomain: "swmscp-9078d.firebaseapp.com",
  projectId: "swmscp-9078d",
  storageBucket: "swmscp-9078d.appspot.com",
  messagingSenderId: "952385770431",
  appId: "1:952385770431:web:927981bfb62d37ccc7870a",
  measurementId: "G-J3V99FKKD5"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
var database = firebase.database();

// References to data
var dataRefSoilMoisture = database.ref('SoilMoisture/Percent');
var dataRefHumidity = database.ref('DHT/humidity');
var dataRefTemperature = database.ref('DHT/temperature');
var dataRefNPK = {
  nitrogen: database.ref('NPK/nitrogen'),
  phosphorus: database.ref('NPK/phosphorus'),
  potassium: database.ref('NPK/potassium')
};

// Fetch data for cards and chart
function fetchData() {
  // Soil Moisture for card
  dataRefSoilMoisture.on('value', function (snapshot) {
    var mois = snapshot.val();
    document.getElementById("soilMoisture").innerHTML = mois + "%";
    storeDataInFirebase('moisture', mois);
  });

  // Humidity for card
  dataRefHumidity.on('value', function (snapshot) {
    var humi = snapshot.val();
    document.getElementById('humidity').innerHTML = humi + "%";
    storeDataInFirebase('humidity', humi);
  });

  // Temperature for card
  dataRefTemperature.on('value', function (snapshot) {
    var temp = snapshot.val();
    document.getElementById('temperature').innerHTML = temp + "&#8451;";
    storeDataInFirebase('temperature', temp);
  });

  // NPK for chart
  dataRefNPK.nitrogen.on('value', updateNPKChart);
  dataRefNPK.phosphorus.on('value', updateNPKChart);
  dataRefNPK.potassium.on('value', updateNPKChart);
}

// Store data in Firebase for later retrieval
function storeDataInFirebase(type, value) {
  const timestamp = new Date().toISOString();
  database.ref(`${type}/`).push({ value, timestamp });
}

// Chart setup
const ctx = document.getElementById('area-chart').getContext('2d');
const areaChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        label: 'Nitrogen',
        data: [],
        backgroundColor: 'rgba(255, 99, 132, 0.4)',
        borderColor: 'rgba(255, 99, 132, 1)',
        fill: true
      },
      {
        label: 'Phosphorus',
        data: [],
        backgroundColor: 'rgba(54, 162, 235, 0.4)',
        borderColor: 'rgba(54, 162, 235, 1)',
        fill: true
      },
      {
        label: 'Potassium',
        data: [],
        backgroundColor: 'rgba(75, 192, 192, 0.4)',
        borderColor: 'rgba(75, 192, 192, 1)',
        fill: true
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      y: {
        beginAtZero: true,
        ticks: { color: '#f5f7ff' },
        title: { display: true, text: 'Nutrient Level (ppm)', color: '#f5f7ff' }
      },
      x: {
        ticks: { color: '#f5f7ff' },
        title: { display: true, text: 'Time', color: '#f5f7ff' }
      }
    },
    plugins: {
      legend: {
        labels: { color: '#f5f7ff' }
      },
      zoom: {
        pan: {
          enabled: true,
          mode: 'x',
        },
        zoom: {
          enabled: true,
          mode: 'x',
        }
      }
    }
  }
});

// Update chart with NPK data
function updateNPKChart(snapshot) {
  var dataKey = snapshot.ref.key;
  var value = snapshot.val();
  var timestamp = new Date().toLocaleString();

  if (dataKey === 'nitrogen') {
    areaChart.data.datasets[0].data.push(value);
  } else if (dataKey === 'phosphorus') {
    areaChart.data.datasets[1].data.push(value);
  } else if (dataKey === 'potassium') {
    areaChart.data.datasets[2].data.push(value);
  }

  areaChart.data.labels.push(timestamp);
  areaChart.update();
}

// Fetch historical data
function fetchHistoricalData() {
  const types = ['nitrogen', 'phosphorus', 'potassium'];
  types.forEach(type => {
    database.ref(`NPK/${type}`).once('value', function(snapshot) {
      snapshot.forEach(function(childSnapshot) {
        let data = childSnapshot.val();
        updateNPKChart({ ref: { key: type }, val: () => data.value });
      });
    });
  });
}

// Download data as CSV
function downloadData() {
  let csvContent = "data:text/csv;charset=utf-8,";
  csvContent += "Timestamp,Soil Moisture,Humidity,Temperature,Nitrogen,Phosphorus,Potassium\n";

  // Retrieve Soil Moisture historical data
  database.ref('SoilMoisture').once('value', function(snapshotSoilMoisture) {
    snapshotSoilMoisture.forEach(function(childSnapshot) {
      const soilData = childSnapshot.val();

      // Retrieve DHT (Humidity and Temperature) historical data
      database.ref('DHT').once('value', function(snapshotDHT) {
        snapshotDHT.forEach(function(dhtSnapshot) {
          const dhtData = dhtSnapshot.val();

          // Retrieve NPK historical data
          database.ref('NPK').once('value', function(snapshotNPK) {
            snapshotNPK.forEach(function(npkSnapshot) {
              const npkData = npkSnapshot.val();

              // Construct a CSV row with the retrieved data
              let row = `${soilData.timestamp || ''},${soilData.Percent || ''},${dhtData.humidity || ''},${dhtData.temperature || ''},${npkData.nitrogen || ''},${npkData.phosphorus || ''},${npkData.potassium || ''}\n`;

              // Append the row to the CSV content
              csvContent += row;
            });
          });
        });
      });
    });

    // After all data is fetched and appended, trigger CSV download
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "Sensor_Datasets.csv");
    document.body.appendChild(link);
    link.click();
  });
}


// Call functions on page load
fetchData();
fetchHistoricalData();

// Add event listener for CSV download
document.getElementById('download-button').addEventListener('click', downloadData);

