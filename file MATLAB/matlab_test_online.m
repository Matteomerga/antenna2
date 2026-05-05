%CODICE ONLINE PER TEST IN PISTA
close all 
clc
clear 
%=======================================================COSTANTI
n_pacchetti = 10;   %pacchetti al secondo
timeplotted = 20;    %ultimi tot secondi che si vogliono visualizzare
delta = 1;    %tempo di aggiornamento in secondi

voltage = 2;  % n° delle COLONNE delle grandezze nel file excel
battCurr = 3;
motorCurr = 4;
speed = 5;
lat = 6;
lon = 7;

FILE = "simulazione1.csv";

%==================================================INIZIO DEL CICLO
cputime_prev = cputime;
pause(delta)
while true
    while cputime - cputime_prev < delta         %aspetta delta secondi prima di plottare di nuovo
    end
    cputime_prev = cputime;
    if true  
        Data=readmatrix(FILE);  
        tot_timebase = Data(:,1)/1000000;
        endbuffer = length(tot_timebase);
        startbuffer = endbuffer - (n_pacchetti * timeplotted); 
        if startbuffer < 1
            startbuffer = 1;
        end
        timebase = tot_timebase(startbuffer:endbuffer)/1000000;
        databuffer = Data(startbuffer:endbuffer, :);
    end
    primo_ciclo = 0;

    
    %BATTERY VOLTAGE
    figure(1)
    subplot(5,1,1)
    plot(timebase,databuffer(:,voltage),'.')
    xlabel('time s')
    ylabel('tensione batteria[V]')
    grid on 
    
    %BATTERY CURRENT
    subplot(5,1,2)
    plot(timebase,databuffer(:,battCurr),'.')
    xlabel('time s')
    ylabel('corrente batteria [A]')
    grid on 
    
    %MOTOR CURRENT
    subplot(5,1,3)
    plot(timebase,databuffer(:,motorCurr),'.')
    xlabel('time s')
    ylabel('corrente motore [A]')
    
    %SPEED
    grid on 
    subplot(5,1,4)
    plot(timebase,databuffer(:,speed),'.')
    xlabel('time s')
    ylabel('speed [km/h]')
     ylim([0,35]);
     grid on 
    
    %GPS
    subplot(5,1,5)
    plot(databuffer(:,lat),databuffer(:,lon),'.')
    xlabel('latitudine')
    ylabel('longitudine')
    
    %GPS SPEED
    %lat_metri =
    %lon_metri = 
    %v_lat = gradient(lat_metri, timebase);
    %v_lon = gradient(lon_metri, timebase);
    %GPSspeed = sqrt(v_lat^2 + v_lon^2);
end









%CALCOLO DELL'ENERGIA
Data = readmatrix(FILE);
timebase = Data(:,1)/1000000
t_start = 3  %INSERIRE ISTANTE INIZIALE
t_end = 4    %INSERIRE ISTANTE FINALE
i_start = find(abs(timebase - t_start) < 1,  1);         %cerca l'indice di t_start; l'uguaglianza è considerata vera nell'intorno di un secondo (aumentare raggio dell'intorno se arriva meno di un pacchetto al secondo)
i_end = find(abs(timebase - t_end) < 1,  1);
W = Data(i_start:i_end, 2).*Data(i_start:i_end, 3);  % Watt = V * A
E = trapz(timebase(i_start:i_end), W)   /  3.6*1e-6;    %conversione Joule-kWh;  SCEGLIERE VALORE FINALE E INIZIALE DI INTEGRAZIONE


