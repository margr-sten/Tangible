import cv2
import numpy as np
import serial
import time
 
 
# åpen serial connection til Arduino
ser = serial.Serial('/dev/cu.usbmodem1401', 9600, timeout=1)#hvis det ikke funker kan det hende du har feil port. sjekk miro koden jeg har brukt på hvordan man sjekker hvilken port som blir burkt
time.sleep(2)  # Wait for the serial connection to initialize
 
# Start videoinnhenting fra iPhone-kamera
cap = cv2.VideoCapture(0)  # Endre tallet om nødvendig
 
# Justerbare variabler for rutenett
rute_bredde = 100  # Bredden på hver rute
rute_hoyde = 100   # Høyden på hver rute
hor_avstand = 221  # Horisontal avstand mellom rutene
ver_avstand = 68  # Vertikal avstand mellom radene
 
# Antall ruter per rad (4, 5, 4, 5, 4)
rader = [4, 5, 4, 5, 4]
 
# Funksjon for å beregne rutenett og midtstille det
def beregn_ruter(frame_shape, rader, rute_bredde, rute_hoyde, hor_avstand, ver_avstand):
    ruter = []
    total_hoyde = len(rader) * rute_hoyde + (len(rader) - 1) * ver_avstand
 
    # Beregn startpunkt for å midtstille rutenettet vertikalt
    start_y = (frame_shape[0] - total_hoyde) // 2
 
    for rad_nummer, antall_ruter in enumerate(rader):
        total_bredde = antall_ruter * rute_bredde + (antall_ruter - 1) * hor_avstand
        
        # Beregn startpunkt for å midtstille ruten i hver rad horisontalt
        start_x = (frame_shape[1] - total_bredde) // 2
 
        for i in range(antall_ruter):
            x = start_x + i * (rute_bredde + hor_avstand)
            y = start_y + rad_nummer * (rute_hoyde + ver_avstand)
            ruter.append((x, y, rute_bredde, rute_hoyde))
    
    return ruter
 
# Funksjon for å detektere hvite sirkler
def detect_white_circles(frame):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (15, 15), 0)
    circles = cv2.HoughCircles(
        blurred,
        cv2.HOUGH_GRADIENT,
        dp=1.2,
        minDist=30,
        param1=50,
        param2=30,
        minRadius=5,
        maxRadius=40
    )
 
    detekterte_sirkler = []
 
    if circles is not None:
        circles = np.round(circles[0, :].astype("int"))
        for (x, y, r) in circles:
            cv2.circle(frame, (x, y), r, (0, 255, 0), 4)  # Tegn sirkel
            cv2.circle(frame, (x, y), 2, (0, 0, 255), 3)  # Tegn sentrum
            detekterte_sirkler.append((x, y, r))
    
    return frame, detekterte_sirkler
 
# Variabel for å lagre forrige status og tidspunkt for endring
forrige_status = []
tidspunkt_siste_endring = time.time()
stabiliseringstid = 2  # Antall sekunder status må være stabil
 
# Funksjon for å sjekke om det er en hvit kule i hver rute
def sjekk_ruter(frame, ruter, sirkler):
    status = []
    
    # Check each grid cell to see if a ball is detected.
    for (x, y, w, h) in ruter:
        kule_i_rute = False
 
        for (cx, cy, r) in sirkler:
            if x < cx < x+w and y < cy < y+h:
                kule_i_rute = True
                break
        
        if kule_i_rute:
            status.append("Opptatt")
            farge = (0, 255, 0)  # Green for occupied
        else:
            status.append("Tom")
            farge = (0, 0, 255)  # Red for empty
        
        # Draw the cell rectangle and write status
        cv2.rectangle(frame, (x, y), (x+w, y+h), farge, 2)
        cv2.putText(frame, status[-1], (x+5, y+30), cv2.FONT_HERSHEY_SIMPLEX, 0.5, farge, 2)
    
    # Print grid cell status
    #for idx, stat in enumerate(status):
       #print(f"Rute {idx+1}: {stat}")
    
    # Create a binary string representation (e.g., "10101")
    output = ''.join(['1' if stat == "Opptatt" else '0' for stat in status])
    print(f"LED Status: {output}")
    
    # Check for changes in status
    global forrige_status, tidspunkt_siste_endring
    if status != forrige_status:
        tidspunkt_siste_endring = time.time()
        forrige_status = status.copy()
    # If the grid status has been stable for the desired period, send a serial prompt
    if time.time() - tidspunkt_siste_endring >= stabiliseringstid:
        # Optionally, send detailed information about which cells are occupied
        occupied_cells = [str(idx+1) for idx, stat in enumerate(status) if stat == "Opptatt"]
        if occupied_cells:
            serial_prompt = "ball_detected:" + ",".join(occupied_cells) + "\n"
        else:
            serial_prompt = "no_ball\n"
        
    print(f"SENDER: {output}")
    #ser.write(serial_prompt.encode('utf-8'))
    output = output + "\n"
    ser.write(output.encode())
    time.sleep(0.1)
 
    return frame, status
 
 
while True:
    ret, frame = cap.read()
    if not ret:
        break
    
    # Beregn og midtstill rutenettet
    ruter = beregn_ruter(frame.shape, rader, rute_bredde, rute_hoyde, hor_avstand, ver_avstand)
    
    # Detekter og tegn hvite sirkler
    frame, sirkler = detect_white_circles(frame)
    
    # Sjekk status for alle rutene
    frame, status = sjekk_ruter(frame, ruter, sirkler)
    
    # Vis resultatet
    cv2.imshow("Rutedeteksjon", frame)
    
    # Avslutt med å trykke på 'q'
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
 
cap.release()
cv2.destroyAllWindows()
 