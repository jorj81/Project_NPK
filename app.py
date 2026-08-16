from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify
from werkzeug.security import generate_password_hash, check_password_hash
from database import get_db_connection
import threading
import time
import requests  # FIXED: Added this missing import

app = Flask(__name__)
app.secret_key = 'project_npk_secure_key'

# ==========================================
# BACKGROUND LOGGER (Talking to ESP32)
# ==========================================
def background_data_logger():
    print("--- BACKGROUND THREAD IS STARTING ---")
    
    ESP_IP = "192.168.254.200" 
    ESP_URL = f"http://{ESP_IP}/data"
    
    while True:
        print(f"DEBUG: Attempting to contact ESP32 at {ESP_URL}...")
        try:
            # Request data from ESP32
            response = requests.get(ESP_URL, timeout=5)
            
            if response.status_code == 200:
                data = response.json()
                print(f"✅ SUCCESS: Received {data}")
                
                # Save to MySQL Database
                conn = get_db_connection()
                if conn:
                    cursor = conn.cursor()
                    query = "INSERT INTO soil_data (nitrogen, phosphorus, potassium, moisture) VALUES (%s, %s, %s, %s)"
                    cursor.execute(query, (data['N'], data['P'], data['K'], data['Moisture']))
                    conn.commit()
                    cursor.close()
                    conn.close()
                    print("DEBUG: Data saved to MySQL.")
            else:
                print(f"❌ SERVER ERROR: ESP32 responded with {response.status_code}")

        except requests.exceptions.RequestException as e:
            print(f"⚠️ NETWORK ERROR: Could not reach ESP32. Reason: {e}")
        except Exception as e:
            print(f"⚠️ GENERAL ERROR: {e}")

        # Wait for 5 seconds (Change to 300 for 5 minutes later)
        print("DEBUG: Sleeping for 5 seconds...\n")
        time.sleep(15)

# ==========================================
# ROUTES (Web Pages)
# ==========================================

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        hashed_password = generate_password_hash(password)
        conn = get_db_connection()
        cursor = conn.cursor()
        try:
            cursor.execute("INSERT INTO users (username, password) VALUES (%s, %s)", (username, hashed_password))
            conn.commit()
            flash("Registration Successful! Please Login.", "success")
            return redirect(url_for('login'))
        except:
            flash("Username already exists!", "danger")
        finally:
            cursor.close()
            conn.close()
    return render_template('register.html')

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT * FROM users WHERE username = %s", (username,))
        user = cursor.fetchone()
        cursor.close()
        conn.close()
        if user and check_password_hash(user['password'], password):
            session['user_id'] = user['id']
            session['username'] = user['username']
            return redirect(url_for('moisture_dashboard'))
        else:
            flash("Invalid Username or Password", "danger")
    return render_template('login.html')

@app.route('/dashboard/moisture')
def moisture_dashboard():
    if 'user_id' not in session: return redirect(url_for('login'))
    return render_template('moisture.html')

@app.route('/dashboard/npk')
def npk_dashboard():
    if 'user_id' not in session: return redirect(url_for('login'))
    return render_template('npk.html')

@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('index'))

@app.route('/api/data')
def get_data():
    if 'user_id' not in session: return jsonify({"error": "Unauthorized"}), 401
    conn = get_db_connection()
    cursor = conn.cursor(dictionary=True)
    cursor.execute("SELECT * FROM soil_data ORDER BY timestamp DESC LIMIT 20")
    rows = cursor.fetchall()
    cursor.close()
    conn.close()
    return jsonify(rows[::-1])

# ==========================================
# START THE SYSTEM
# ==========================================
if __name__ == '__main__':
    # Start the background thread
    data_thread = threading.Thread(target=background_data_logger, daemon=True)
    data_thread.start()
    
    # Start Flask (use_reloader=False prevents the thread from starting twice)
    app.run(debug=True, use_reloader=False, host='0.0.0.0', port=5000)
