import cv2
import mediapipe as mp
import joblib
import numpy as np
import os
import time
import json
from collections import deque
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient

# ==========================================
# --- AWS IOT CORE CONFIGURATION ---
# ==========================================
AWS_ENDPOINT = "a260nvvzszz66f-ats.iot.us-east-1.amazonaws.com" 
AWS_PORT = 8883
AWS_CLIENT_ID = "DeskBot_Laptop"
AWS_TOPIC = "deskbot/posture"

ROOT_CA_PATH = "AmazonRootCA1.pem"
PRIVATE_KEY_PATH = "db8eb26a2499f06fb749dbd05764d4a2f6cdccc53e7142f7617dc8463bfcc2a8-private.pem.key"
CERTIFICATE_PATH = "db8eb26a2499f06fb749dbd05764d4a2f6cdccc53e7142f7617dc8463bfcc2a8-certificate.pem.crt"

print("Connecting to AWS IoT Core...")
mqtt_client = AWSIoTMQTTClient(AWS_CLIENT_ID)
mqtt_client.configureEndpoint(AWS_ENDPOINT, AWS_PORT)
mqtt_client.configureCredentials(ROOT_CA_PATH, PRIVATE_KEY_PATH, CERTIFICATE_PATH)
mqtt_client.configureAutoReconnectBackoffTime(1, 32, 20)
mqtt_client.configureOfflinePublishQueueing(-1)  
mqtt_client.configureDrainingFrequency(2)  
mqtt_client.configureConnectDisconnectTimeout(10)  
mqtt_client.configureMQTTOperationTimeout(5)  

try:
    mqtt_client.connect()
    print("Successfully connected to AWS IoT Core!")
except Exception as e:
    print(f"AWS Connection Failed. Check certificates and endpoint. Error : {e}")
    exit()
# ==========================================

def calculate_geometric_features(lm_list):
    """Applies the exact same math used during training to the live webcam feed."""
    nose = np.array([lm_list[0].x, lm_list[0].y])
    shoulder_l = np.array([lm_list[11].x, lm_list[11].y])
    shoulder_r = np.array([lm_list[12].x, lm_list[12].y])
    
    shoulder_width = np.linalg.norm(shoulder_l - shoulder_r)
    shoulder_midpoint = (shoulder_l + shoulder_r) / 2.0
    neck_length = np.linalg.norm(nose - shoulder_midpoint)
    slouch_ratio = neck_length / (shoulder_width + 1e-6) 
    
    delta_y = shoulder_r[1] - shoulder_l[1]
    delta_x = shoulder_r[0] - shoulder_l[0]
    shoulder_tilt_angle = np.degrees(np.arctan2(delta_y, delta_x))

    features = [
        shoulder_width, neck_length, slouch_ratio, shoulder_tilt_angle,
        lm_list[0].x, lm_list[0].y,
        lm_list[1].x, lm_list[1].y,
        lm_list[2].x, lm_list[2].y,
        lm_list[11].x, lm_list[11].y,
        lm_list[12].x, lm_list[12].y
    ]
    return features

def main():
    model_path = 'posture_model_v3.pkl' # USING THE NEW 98% ACCURACY MODEL!

    try:
        if not os.path.exists(model_path):
            raise FileNotFoundError(f"Error: '{model_path}' not found.")

        model_data = joblib.load(model_path)
        model = model_data['model']
        scaler = model_data['scaler']

        pose_model_path = 'pose_landmarker_lite.task'
        BaseOptions = mp.tasks.BaseOptions
        PoseLandmarker = mp.tasks.vision.PoseLandmarker
        PoseLandmarkerOptions = mp.tasks.vision.PoseLandmarkerOptions
        VisionRunningMode = mp.tasks.vision.RunningMode

        options = PoseLandmarkerOptions(
            base_options=BaseOptions(model_asset_path=pose_model_path),
            running_mode=VisionRunningMode.VIDEO,
            min_pose_detection_confidence=0.6,
            min_tracking_confidence=0.6
        )

        with PoseLandmarker.create_from_options(options) as landmarker:
            cap = cv2.VideoCapture(0) 
            if not cap.isOpened(): return

            print("Starting real-time AI vision...")
            timestamp_ms = 0
            
            # Smoothing & AWS Variables
            prediction_history = deque(maxlen=30) # Remembers the last 1 second of frames
            last_sent_state = ""
            last_send_time = time.time()

            while cap.isOpened():
                ret, frame = cap.read()
                if not ret: break

                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                result = landmarker.detect_for_video(mp_image, timestamp_ms)
                timestamp_ms += 33  

                raw_prediction = "Detecting..."
                smoothed_prediction = "Detecting..."

                if result.pose_landmarks:
                    lm = result.pose_landmarks[0]
                    
                    # 1. Apply our mathematical feature engineering
                    engineered_features = calculate_geometric_features(lm)

                    # 2. Scale and Predict
                    features_scaled = scaler.transform([engineered_features])
                    raw_prediction = model.predict(features_scaled)[0]
                    
                    # 3. Temporal Smoothing (The Pen Drop Fix)
                    prediction_history.append(raw_prediction)
                    # The smoothed state is whatever posture occurred MOST in the last 30 frames
                    smoothed_prediction = max(set(prediction_history), key=prediction_history.count)

                    # ==========================================
                    # --- AWS PUBLISH LOGIC ---
                    # ==========================================
                    current_time = time.time()
                    
                    # Only trigger AWS if the SMOOTHED posture changes, or every 3 seconds as a heartbeat
                    if smoothed_prediction != last_sent_state or (current_time - last_send_time > 3.0):
                        payload = json.dumps({"state": smoothed_prediction})
                        try:
                            mqtt_client.publish(AWS_TOPIC, payload, 1)
                            print(f"AWS Sync -> {payload}")
                            last_sent_state = smoothed_prediction
                            last_send_time = current_time
                        except Exception as e:
                            print(f"AWS Publish Failed: {e}")
                    # ==========================================
                else:
                    # If nobody is in the frame, gently fall back to Not At Desk
                    prediction_history.append("Not At Desk")
                    if len(prediction_history) > 0:
                        smoothed_prediction = max(set(prediction_history), key=prediction_history.count)

                # UI Display
                cv2.putText(frame, f"AI State: {smoothed_prediction}", (10, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)
                cv2.imshow('DeskBot AI Vision (v3)', frame)

                if cv2.waitKey(1) & 0xFF == ord('q'): break

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        if 'cap' in locals() and cap.isOpened(): cap.release()
        cv2.destroyAllWindows()
        mqtt_client.disconnect()
        print("Disconnected from AWS.")

if __name__ == "__main__":
    main()