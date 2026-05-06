%CODICE ONLINE PER TEST IN PISTA
close all 
clc
clear 
%=======================================================COSTANTI
n_pacchetti = 10;   %pacchetti al secondo
timeplotted = 20;    %ultimi tot secondi che si vogliono visualizzare
delta = 2;    %tempo di aggiornamento in secondi

voltage = 2;  % n° della COLONNA delle grandezze nel file excel
battCurr = 3;
motorCurr = 4;
speed = 5;
lat = 6;
lon = 7;

FILE = "serial_data.csv";

%==================================================VISUALIZZAZIONE DELLA FINESTRA
cputime_prev = cputime;
pause(delta)
figure('WindowState','maximized')
t = tiledlayout(5,1,'Padding','compact','TileSpacing','compact');
ax1 = nexttile;
ax2 = nexttile;
ax3 = nexttile;
ax4 = nexttile;
ax5 = nexttile;
zoom on
pan on

%==================================================INIZIO DEL CICLO
while true
    while cputime - cputime_prev < delta         %aspetta delta secondi prima di plottare di nuovo
    end
    cputime_prev = cputime;  
    Data=readmatrix(FILE);  
    tot_timebase = Data(:,1)/1000000;
    endbuffer = length(tot_timebase);
    startbuffer = endbuffer - (n_pacchetti * timeplotted); 
    if startbuffer < 1
        startbuffer = 1;
    end
    timebase = tot_timebase(startbuffer:endbuffer)/1000000;
    databuffer = Data(startbuffer:endbuffer, :);

    
    %BATTERY VOLTAGE
    plot(ax1, timebase,databuffer(:,voltage),'.')
    xlabel('time s')
    ylabel(ax1, 'tensione batteria[V]')
    grid on 
    
    %BATTERY CURRENT
    plot(ax2, timebase,databuffer(:,battCurr),'.')
    xlabel('time s')
    ylabel(ax2, 'corrente batteria [A]')
    grid on 
    
    %MOTOR CURRENT
    plot(ax3, timebase,databuffer(:,motorCurr),'.')
    xlabel('time s')
    ylabel(ax3, 'corrente motore [A]')
    grid on
    
    %SPEED
    plot(ax4, timebase,databuffer(:,speed),'.')
    xlabel('time s')
    ylabel(ax4, 'speed [km/h]')
     ylim([0,35]);
     grid on 
    
    %GPS
    plot(ax5, databuffer(:,lat),databuffer(:,lon),'.')
    xlabel(ax5, 'latitudine')
    ylabel(ax5, 'longitudine')
    
    %GPS SPEED
    %lat_metri =
    %lon_metri = 
    %v_lat = gradient(lat_metri, timebase);
    %v_lon = gradient(lon_metri, timebase);
    %GPSspeed = sqrt(v_lat^2 + v_lon^2);

    drawnow
    pause(0.1)
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



