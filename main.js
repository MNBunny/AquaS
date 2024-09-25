document.addEventListener("DOMContentLoaded", function () {
  const menuIcon = document.querySelector('.menu-icon');
  const sidebar = document.getElementById('sidebar');

  menuIcon.addEventListener('click', function () {
    sidebar.classList.toggle('sidebar-responsive');
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

// Getting reference to the data
var dataRef1 = database.ref('SoilMoisture/Percent');
var dataRef2 = database.ref('DHT/humidity');
var dataRef3 = database.ref('DHT/temperature');

// Fetch data for display and chart
function fetchData() {
  dataRef1.on('value', function (getdata1) {
    var mois = getdata1.val();
    document.getElementById("soilMoisture").innerHTML = mois + "%";
    updateChartData(areaChart, mois);
    storeDataInFirebase('moisture', mois);
  });

  dataRef2.on('value', function (getdata2) {
    var humi = getdata2.val();
    document.getElementById('humidity').innerHTML = humi + "%";
    storeDataInFirebase('humidity', humi);
  });

  dataRef3.on('value', function (getdata3) {
    var temp = getdata3.val();
    document.getElementById('temperature').innerHTML = temp + "&#8451;";
    storeDataInFirebase('temperature', temp);
  });
}

// Store data for the past 3 months
function storeDataInFirebase(type, value) {
  const timestamp = new Date().toISOString();
  database.ref(`${type}/`).push({ value, timestamp });
}

// Initial chart setup
const ctx = document.getElementById('area-chart').getContext('2d');
const areaChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [{
      label: 'Moisture Level',
      data: [],
      backgroundColor: 'rgba(0, 171, 87, 0.4)',
      borderColor: 'rgba(0, 171, 87, 1)',
      borderWidth: 1,
      fill: true,
      tension: 0.2
    }]
  },
  options: {
    responsive: true,
    scales: {
      y: {
        beginAtZero: true,
        ticks: { color: '#f5f7ff' },
        title: { display: true, text: 'Moisture Level', color: '#f5f7ff' }
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

// Fetch historical data on page load
function fetchHistoricalData() {
  dataRef1.once('value', function(snapshot) {
    snapshot.forEach(function(childSnapshot) {
      let data = childSnapshot.val();
      areaChart.data.datasets[0].data.push(data.value);
      areaChart.data.labels.push(new Date(data.timestamp).toLocaleString());
    });
    areaChart.update();
  });
}

// Download data as CSV
function downloadData() {
  let csvContent = "data:text/csv;charset=utf-8,";
  csvContent += "Timestamp,Moisture Level,Humidity,Temperature\n";

  const types = ['moisture', 'humidity', 'temperature'];
  types.forEach(type => {
    database.ref(`${type}/`).once('value', function (snapshot) {
      snapshot.forEach(function (childSnapshot) {
        let data = childSnapshot.val();
        csvContent += `${data.timestamp},${data.value},${data.humidity || ''},${data.temperature || ''}\n`;
      });
    });
  });

  const encodedUri = encodeURI(csvContent);
  const link = document.createElement("a");
  link.setAttribute("href", encodedUri);
  link.setAttribute("download", "data.csv");
  document.body.appendChild(link);
  link.click();
}

// Call functions
fetchData();
fetchHistoricalData();

// Add an event listener for downloading data
document.getElementById('download-button').addEventListener('click', downloadData);
