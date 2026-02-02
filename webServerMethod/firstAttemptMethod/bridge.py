from flask import Flask, request, jsonify
from flask_cors import CORS
from pymongo import MongoClient
from datetime import datetime
import json

app = Flask(__name__)
CORS(app)

# MongoDB Connection
client = MongoClient('mongodb://localhost:27017/')
db = client['pushup_tracker']
sessions_collection = db['sessions']
stats_collection = db['lifetime_stats']

@app.route('/store', methods=['POST'])
def store_data():
    try:
        data = request.json
        data['received_at'] = datetime.now()
        
        # Store session
        result = sessions_collection.insert_one(data)
        
        # Update lifetime statistics
        update_lifetime_stats(data)
        
        return jsonify({"status": "success", "id": str(result.inserted_id)})
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)})

def update_lifetime_stats(session_data):
    """Update all-time statistics"""
    stats = stats_collection.find_one({"_id": "lifetime"})
    
    if not stats:
        # First time setup
        stats_collection.insert_one({
            "_id": "lifetime",
            "total_pushups": session_data.get('pushupCount', 0),
            "total_calories": session_data.get('totalCalories', 0),
            "total_sessions": 1,
            "best_session": session_data.get('pushupCount', 0),
            "last_updated": datetime.now()
        })
    else:
        # Update existing stats
        stats_collection.update_one(
            {"_id": "lifetime"},
            {"$inc": {
                "total_pushups": session_data.get('pushupCount', 0),
                "total_calories": session_data.get('totalCalories', 0),
                "total_sessions": 1
            },
             "$max": {"best_session": session_data.get('pushupCount', 0)},
             "$set": {"last_updated": datetime.now()}}
        )

@app.route('/stats', methods=['GET'])
def get_stats():
    """Get lifetime statistics"""
    stats = stats_collection.find_one({"_id": "lifetime"})
    sessions = list(sessions_collection.find().sort("timestamp", -1).limit(10))
    
    # Convert ObjectId to string for JSON serialization
    for session in sessions:
        session['_id'] = str(session['_id'])
    
    return jsonify({
        "lifetime_stats": stats,
        "recent_sessions": sessions
    })

@app.route('/weekly', methods=['GET'])
def get_weekly():
    """Get weekly progress"""
    week_ago = datetime.now() - timedelta(days=7)
    weekly_data = sessions_collection.find({"received_at": {"$gte": week_ago}})
    
    days = {}
    for session in weekly_data:
        day = session['received_at'].strftime("%A")
        days[day] = days.get(day, 0) + session.get('pushupCount', 0)
    
    return jsonify(days)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)