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

// Store data in Firebase with auto-increment ID
function storeDataInFirebase(type, value) {
  const timestamp = new Date().toISOString();
  
  // Reference to the counter for the type
  var counterRef = database.ref(`${type}/counter`);
  
  // Get the current count (auto-increment ID)
  counterRef.transaction(function (currentValue) {
    // Increment the counter or initialize it if it's the first value
    return (currentValue || 0) + 1;
  }).then(function (result) {
    const newId = result.snapshot.val();  // Get the incremented ID
    
    // Store data with the new auto-incremented ID
    database.ref(`${type}/data/${newId}`).set({ 
      value, 
      timestamp 
    });
  }).catch(function (error) {
    console.log("Error incrementing counter:", error);
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
  const types = ['moisture', 'humidity', 'temperature', 'nitrogen', 'phosphorus', 'potassium'];
  
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


// Download all stored historical data as CSV, including auto-incremented ID
function downloadData() {
  let csvContent = "data:text/csv;charset=utf-8,";
  csvContent += "ID,Timestamp,Soil Moisture,Humidity,Temperature,Nitrogen,Phosphorus,Potassium\n";

  // Define types of data to be fetched
  const types = ['moisture', 'humidity', 'temperature', 'nitrogen', 'phosphorus', 'potassium'];
  
  // Log Firebase data retrieval steps
  console.log("Attempting to fetch historical data from Firebase...");

  // Use a promise to ensure that all data is fetched before triggering the download
  let promises = [];

  // Iterate over each type of data (moisture, humidity, etc.)
  types.forEach(type => {
    const promise = database.ref(`${type}/data`).once('value').then(snapshot => {
      snapshot.forEach(function (childSnapshot) {
        const data = childSnapshot.val();
        const id = childSnapshot.key;  // The auto-incremented ID
        const timestamp = data.timestamp || '';  // Timestamp for the data entry

        // Initialize variables for each sensor data type
        let soilMoisture = (type === 'moisture') ? data.value || '' : '';
        let humidity = (type === 'humidity') ? data.value || '' : '';
        let temperature = (type === 'temperature') ? data.value || '' : '';
        let nitrogen = (type === 'nitrogen') ? data.value || '' : '';
        let phosphorus = (type === 'phosphorus') ? data.value || '' : '';
        let potassium = (type === 'potassium') ? data.value || '' : '';

        // Add a CSV row that includes the auto-incremented ID and all sensor values
        let row = `${id},${timestamp},${soilMoisture},${humidity},${temperature},${nitrogen},${phosphorus},${potassium}\n`;
        csvContent += row;
      });
    });
    promises.push(promise);
  });

  // After all data is fetched, trigger CSV download
  Promise.all(promises).then(() => {
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
