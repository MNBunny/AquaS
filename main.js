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
var dataRefSoilMoisture1 = database.ref('SoilMoisture/Percent_1');
var dataRefSoilMoisture2 = database.ref('SoilMoisture/Percent_2');
var dataRefHumidity = database.ref('DHT/humidity');
var dataRefTemperature = database.ref('DHT/temperature');
var dataRefNPK = {
  nitrogen: database.ref('NPK/nitrogen'),
  phosphorus: database.ref('NPK/phosphorus'),
  potassium: database.ref('NPK/potassium')
};

function fetchData() {
  // Soil Moisture for card 1
  dataRefSoilMoisture1.on('value', function (snapshot) {
    var mois1 = snapshot.val();
    document.getElementById("soilMoisture_1").innerHTML = mois1 + "%";

    // Create structured data
    const moistureData1 = {
      value: mois1,
      timestamp: new Date().toISOString() // Adding ISO timestamp for better date handling
    };

    storeDataInFirebase('SoilMoisture/Percent_1', moistureData1, 'Soil Moisture Sensor 1'); // Specify the sensor
  });

  // Soil Moisture for card 2
  dataRefSoilMoisture2.on('value', function (snapshot) {
    var mois2 = snapshot.val();
    document.getElementById("soilMoisture_2").innerHTML = mois2 + "%";

    // Create structured data
    const moistureData2 = {
      value: mois2,
      timestamp: new Date().toISOString() // Adding ISO timestamp for better date handling
    };

    storeDataInFirebase('SoilMoisture/Percent_2', moistureData2, 'Soil Moisture Sensor 2'); // Specify the sensor
  });

  // Humidity for card
  dataRefHumidity.on('value', function (snapshot) {
    var humi = snapshot.val();
    document.getElementById('humidity').innerHTML = humi + "%";
    storeDataInFirebase('humidity', humi, 'Humidity Sensor'); // Specify the sensor
  });

  // Temperature for card
  dataRefTemperature.on('value', function (snapshot) {
    var temp = snapshot.val();
    document.getElementById('temperature').innerHTML = temp + "&#8451;";
    storeDataInFirebase('temperature', temp, 'Temperature Sensor'); // Specify the sensor
  });

  // NPK for chart
  dataRefNPK.nitrogen.on('value', updateNPKChart);
  dataRefNPK.phosphorus.on('value', updateNPKChart);
  dataRefNPK.potassium.on('value', updateNPKChart);
}


function storeDataInFirebase(type, value, sensor) {
  // Reference to the latest data for the type
  var lastEntryRef = database.ref(`${type}/data`).orderByKey().limitToLast(1);

  // Check the latest entry before saving
  lastEntryRef.once('value').then(function (snapshot) {
    const timestamp = new Date();
    const date = timestamp.toLocaleDateString('en-US', { year: '2-digit', month: '2-digit', day: '2-digit' });
    const time = timestamp.toLocaleTimeString('en-US', { hour12: false });

    // Auto-generate a unique key with Firebase push() instead of using a counter
    const newDataRef = database.ref(`${type}/data`).push();

    // Store the new data with the generated key, date, time, and sensor identifier
    newDataRef.set({
      value,
      date,
      time,
      sensor // Include the sensor identifier here
    }).then(() => {
      // Store the current value in session storage for future reference
      sessionStorage.setItem(`${type}-last-value`, value.toString());
    }).catch(function (error) {
      console.error("Error saving data:", error);
    });
  }).catch(function (error) {
    console.error("Error checking last entry:", error);
  });
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

// Fetch historical data from stored records using auto-incremented IDs
function fetchHistoricalData() {
  const types = ['moisture_1', 'moisture_2', 'humidity', 'temperature', 'nitrogen', 'phosphorus', 'potassium'];
  
  types.forEach(type => {
    // Retrieve all historical data stored under the 'data' key
    database.ref(`${type}/data`).once('value', function (snapshot) {
      snapshot.forEach(function (childSnapshot) {
        let data = childSnapshot.val();
        let timestamp = childSnapshot.key;  // The auto-incremented ID
        
        // Use the data for your purposes (display, chart, etc.)
        console.log(`${type} at ${timestamp}:`, data);
        
        // For NPK data, we will need to update the chart
        if (['nitrogen', 'phosphorus', 'potassium'].includes(type)) {
          updateNPKChart({ ref: { key: type }, val: () => data.value });
        }
      });
    });
  });
}

// Modify downloadData to include separate Date and Time columns
function downloadData() {
  let csvContent = "data:text/csv;charset=utf-8,";
  csvContent += "ID,Date,Time,Soil Moisture,Humidity,Temperature,Nitrogen,Phosphorus,Potassium\n";

  const types = ['moisture', 'humidity', 'temperature', 'nitrogen', 'phosphorus', 'potassium'];
  let promises = [];
  let aggregatedData = {};

  // Iterate over each type of data (moisture, humidity, etc.)
  types.forEach(type => {
    const promise = database.ref(`${type}/data`).once('value').then(snapshot => {
      snapshot.forEach(childSnapshot => {
        const data = childSnapshot.val();
        const id = childSnapshot.key;
        const date = data.date || '';  // Date for the data entry
        const time = data.time || '';  // Time for the data entry

        if (!aggregatedData[id]) {
          aggregatedData[id] = { id, date, time };
        }
        aggregatedData[id][type] = data.value;
      });
    });
    promises.push(promise);
  });

  // After all data is fetched, trigger CSV download
  Promise.all(promises).then(() => {
    for (const id in aggregatedData) {
      const entry = aggregatedData[id];
      const row = `${entry.id},${entry.date},${entry.time},${entry.moisture || ''},${entry.humidity || ''},${entry.temperature || ''},${entry.nitrogen || ''},${entry.phosphorus || ''},${entry.potassium || ''}\n`;
      csvContent += row;
    }
    
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "Sensor_Datasets.csv");
    document.body.appendChild(link);
    link.click();
  }).catch(error => {
    console.error("Error fetching data for CSV download:", error);
  });
}


// Call functions on page load
fetchData();
fetchHistoricalData();

// Add event listener for CSV download
document.getElementById('download-button').addEventListener('click', downloadData);
