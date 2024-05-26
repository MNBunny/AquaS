// Import the functions you need from the SDKs you need

// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
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
firebase.initializeApp(firebaseConfig)

// getting reference to the database
var database = firebase.database();


//getting reference to the data we want
var dataRef1 = database.ref('SoilMoisture/Percent');
var dataRef2 = database.ref('DHT/humidity');
var dataRef3 = database.ref('DHT/temperature');

//fetch the data
dataRef1.on('value', function(getdata1) {
  var mois = getdata1.val();
  document.getElementById("soilMoisture").innerHTML = mois + "%";
})
dataRef2.on('value', function(getdata2){
  var humi = getdata2.val();
  document.getElementById('humidity').innerHTML = humi + "%";
})

dataRef3  .on('value', function(getdata3){
  var temp = getdata3.val();
  document.getElementById('temperature').innerHTML = temp + "&#8451;";
})



let soilMoisturePercent = [];
let labels = [];

dataRef1.on('value', function(getdata1) {
  soilMoisturePercent = getdata1.val() || [];
  updateChartData(areaChart, soilMoisturePercent);
});

function updateChartData(chart, newData) {
  chart.data.datasets[0].data.push(newData);
  chart.data.labels.push(new Date().toLocaleTimeString());
  chart.update();
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
        ticks: {
          color: '#f5f7ff'
        },
        title: {
          display: true,
          text: 'Moisture Level',
          color: '#f5f7ff'
        }
      },
      x: {
        ticks: {
          color: '#f5f7ff'
        },
        title: {
          display: true,
          text: 'Time',
          color: '#f5f7ff'
        }
      }
    },
    plugins: {
      legend: {
        labels: {
          color: '#f5f7ff'
        }
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
