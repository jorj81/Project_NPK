import mysql.connector
from mysql.connector import Error   # ✅ tamang import

def get_db_connection():
    try:
        connection = mysql.connector.connect(
            host='localhost',
            user='root',      # Default XAMPP user
            password='',      # Default XAMPP password is empty
            database='project_npk'
        )
        return connection
    except Error as e:
        print(f"Database Connection Error: {e}")
        return None
