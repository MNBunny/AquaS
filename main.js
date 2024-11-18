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
      
    nitrogen: database.ref('NPK/Nitrogen'),
    phosphorus: database.ref('NPK/Phosphorus'),
    potassium: database.ref('NPK/Potassium')
  };
  
  
  
  let moisture1 = 0;
  let moisture2 = 0;
  
  function fetchData() {
      // Soil Moisture Sensor 1
      dataRefSoilMoisture1.on('value', function (snapshot) {
          moisture1 = parseFloat(snapshot.val()) || 0;
          updateSoilMoistureDisplay();
          storeSoilMoistureInFirebase('moisture1', moisture1);
      });
  
      // Soil Moisture Sensor 2
      dataRefSoilMoisture2.on('value', function (snapshot) {
          moisture2 = parseFloat(snapshot.val()) || 0;
          updateSoilMoistureDisplay();
          storeSoilMoistureInFirebase('moisture2', moisture2);
      });
  
      // Humidity
      dataRefHumidity.on('value', function (snapshot) {
          const humi = snapshot.val() || 0;
          document.getElementById('humidity').innerHTML = `${humi}%`;
          storeDataInFirebase('humidity', humi);
      });
  
      // Temperature
      dataRefTemperature.on('value', function (snapshot) {
          const temp = snapshot.val() || 0;
          document.getElementById('temperature').innerHTML = `${temp}&#8451;`;
          storeDataInFirebase('temperature', temp);
      });
  
      // NPK Data Updates
      dataRefNPK.nitrogen.on('value', function(snapshot) {
          const nitrogenValue = snapshot.val() || 0;
          storeDataInFirebase('nitrogen', nitrogenValue);
          updateNPKChart(snapshot);  // Update the chart
      });
  
      dataRefNPK.phosphorus.on('value', function(snapshot) {
          const phosphorusValue = snapshot.val() || 0;
          storeDataInFirebase('phosphorus', phosphorusValue);
          updateNPKChart(snapshot);  // Update the chart
      });
  
      dataRefNPK.potassium.on('value', function(snapshot) {
          const potassiumValue = snapshot.val() || 0;
          storeDataInFirebase('potassium', potassiumValue);
          updateNPKChart(snapshot);  // Update the chart
      });
  
  }
  
  function updateSoilMoistureDisplay() {
      console.log("Moisture 1:", moisture1);
      console.log("Moisture 2:", moisture2);
      const averageMoisture = (moisture1 + moisture2) / 2;
      document.getElementById('soilMoisture').innerHTML = `${averageMoisture}%`;
  }
  
  
  // Call fetchData on page load
  fetchData();
  
  function storeDataInFirebase(type, value) {
      // Retrieve last saved value from session storage
      const lastValue = sessionStorage.getItem(`${type}-last-value`);
  
      // Compare current value with the last saved value
      if (value.toString() !== lastValue) {
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
  
              // Save data for NPK (Nitrogen, Phosphorus, Potassium) in Firebase under NPK parent node
              if (type === 'nitrogen' || type === 'phosphorus' || type === 'potassium') {
                  database.ref(`${type}/data/${newId}`).set({
                      value,
                      date,
                      time
                  });
              } else {
                  // Save general sensor data (like humidity, temperature, soil moisture)
                  database.ref(`${type}/data/${newId}`).set({
                      value,
                      date,
                      time
                  });
              }
  
              console.log(`Saved ${type} data:`, { value, date, time });
  
              // Store the current value in session storage to track the last value
              sessionStorage.setItem(`${type}-last-value`, value.toString());
          }).catch(function (error) {
              console.error("Error saving data:", error);
          });
      } else {
          console.log(`No change in ${type}. Data not saved.`);
      }
  }
  
  
  
  function storeSoilMoistureInFirebase(sensor, value) {
      // Call storeDataInFirebase for soil moisture readings
      storeDataInFirebase(sensor, value);
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
              backgroundColor: 'rgba(255, 104, 89, 0.4)',
              borderColor: 'rgba(255, 104, 89, 0.8)',
              fill: true,
              cubicInterpolationMode: 'monotone',
              tension: 0.8
          },
          {
              label: 'Phosphorus',
              data: [],
              backgroundColor: 'rgba(93, 153, 193, 0.4)',
              borderColor: 'rgba(93, 153, 193, 0.8)',
              fill: true,
              cubicInterpolationMode: 'monotone',
              tension: 0.8
          },
          {
              label: 'Potassium',
              data: [],
              backgroundColor: 'rgba(103, 198, 185, 0.4)',
              borderColor: 'rgba(103, 198, 185, 0.8)',
              fill: true,
              cubicInterpolationMode: 'monotone',
              tension: 0.8
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
  
  async function fetchNPKData() {
      const npkTypes = ['nitrogen', 'phosphorus', 'potassium'];
      const chartLabels = new Set(); // Use a Set to store unique timestamps
  
      try {
          for (const type of npkTypes) {
              // Fetch data from Firebase for each NPK type
              const snapshot = await database.ref(`${type}/data`).once('value');
              const dataRecords = snapshot.val();
  
              if (dataRecords) {
                  Object.entries(dataRecords).forEach(([id, data]) => {
                      const timestamp = new Date(`${data.date} ${data.time}`).toLocaleString();
  
                      // Check if the timestamp has already been added to avoid duplicates
                      if (!chartLabels.has(timestamp)) {
                          chartLabels.add(timestamp);
                          areaChart.data.labels.push(timestamp); // Add the unique timestamp to labels
                      }
  
                      // Update the chart dataset based on NPK type
                      if (type === 'nitrogen') {
                          areaChart.data.datasets[0].data.push(data.value);
                      } else if (type === 'phosphorus') {
                          areaChart.data.datasets[1].data.push(data.value);
                      } else if (type === 'potassium') {
                          areaChart.data.datasets[2].data.push(data.value);
                      }
                  });
              }
          }
          
          // Update the chart once after all data is processed
          areaChart.update();
      } catch (error) {
          console.error("Error fetching NPK data:", error);
      }
  }
  
  // Call this function when the page loads to fetch historical data for NPK
  document.addEventListener("DOMContentLoaded", function () {
      fetchNPKData();
  });

  
  function fetchHistoricalData() {
      const types = ['nitrogen', 'phosphorus', 'potassium'];
      types.forEach(async (type) => {
          try {
              const snapshot = await database.ref(`${type}/data`).once('value');
              const dataRecords = snapshot.val();
              if (dataRecords) {
                  Object.entries(dataRecords).forEach(([id, data]) => {
                      // Call updateNPKChart with each historical data record
                      updateNPKChart({ ref: { key: type }, val: () => data.value });
                  });
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
    const ids = [];  // Array to hold the IDs
  
    types.forEach(type => {
        const dataRef = database.ref(`${type}/data`);
        dataRef.once('value', (snapshot) => {
            const dataRecords = snapshot.val();
  
            if (dataRecords) {
                Object.entries(dataRecords).forEach(([id, data]) => {
                    ids.push(id); // Collect IDs for download
  
                    const row = `${id},${data.date},${data.time},${data.value},${data.value || ''},${data.value || ''},${data.value || ''},${data.value || ''},${data.value || ''}`;
                    csvContent += row + "\n";
                });
            }
        });
    });
  
    // Create a link and trigger download
    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", "sensor_data.csv");
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }
  
  // Call fetchHistoricalData on page load
  fetchHistoricalData();