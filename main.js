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
  // Soil Moisture Sensor 1 for card
  dataRefSoilMoisture1.on('value', function (snapshot) {
    var mois1 = snapshot.val();
    document.getElementById("soilMoisture1").innerHTML = mois1 + "%";
    storeDataInFirebase('moisture1', mois1);
  });

  // Soil Moisture Sensor 2 for card
  dataRefSoilMoisture2.on('value', function (snapshot) {
    var mois2 = snapshot.val();
    document.getElementById("soilMoisture2").innerHTML = mois2 + "%";
    storeDataInFirebase('moisture2', mois2);
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



function storeDataInFirebase(type, value) {
  const timestamp = new Date();
  const date = timestamp.toLocaleDateString('en-US', { year: '2-digit', month: '2-digit', day: '2-digit' });
  const time = timestamp.toLocaleTimeString('en-US', { hour12: false });

  // Firebase counter reference for auto-incrementing the ID
  const counterRef = database.ref(`${type}/counter`);

  // Increment the counter and store the new data
  counterRef.transaction(function (currentValue) {
    return (currentValue || 0) + 1;  // Increment counter or start at 1
  }).then(function (result) {
    const newId = result.snapshot.val();  // Get new incremented ID

    // Store the new data with auto-incremented ID, date, and time
    database.ref(`${type}/data/${newId}`).set({
      value,
      date,
      time
    });

    console.log(`Saved ${type} data:`, { value, date, time });

    // Optional: Store the current value in session storage to track the last value
    sessionStorage.setItem(`${type}-last-value`, value.toString());
  }).catch(function (error) {
    console.error("Error saving data:", error);
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

  types.forEach(async (type) => {
    try {
      // Retrieve all historical data stored under the 'data' key
      const snapshot = await database.ref(`${type}/data`).once('value');
      const dataRecords = snapshot.val();  // Get all records as an object

      if (dataRecords) {
        Object.entries(dataRecords).forEach(([id, data]) => {
          console.log(`${type} at ID ${id}:`, data);  // Log each record

          // If it's NPK data, update the chart
          if (['nitrogen', 'phosphorus', 'potassium'].includes(type)) {
            updateNPKChart({ key: type, value: data.value });
          }
        });
      } else {
        console.log(`No historical data found for ${type}.`);
      }
    } catch (error) {
      console.error(`Error fetching data for ${type}:`, error);
    }
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
