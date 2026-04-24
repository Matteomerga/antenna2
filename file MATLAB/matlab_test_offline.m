%CODICE OFFLINE PER TEST IN PISTA
close all 
clc
clear 

Data=readmatrix("simulazione1.csv");         %inserire nome file.csv da visualizzare
 

timebase = Data(:,1)/1000000;


%BATTERY VOLTAGE
figure(1)
subplot(5,1,1)
plot(timebase,Data(:,2),'.')
xlabel('time micro-s')
ylabel('tensione batteria[V]')
grid on 

%BATTERY CURRENT
subplot(5,1,2)
plot(timebase,Data(:,3),'.')
xlabel('time micro-s')
ylabel('corrente batteria [A]')
grid on 

%MOTOR CURRENT
subplot(5,1,3)
plot(timebase,(Data(:,4)./1023*5-2.5)/0.185,'.')
xlabel('time micro-s')
ylabel('corrente motore [A]')

%SPEED
grid on 
subplot(5,1,4)
plot(timebase,Data(:,5).*2*pi/60*0.28*3.6,'.')
xlabel('time micro-s')
ylabel('speed [km/h]')
 ylim([0,35]);
 grid on 

%GPS
subplot(5,1,5)
plot(Data(:,6),Data(:,7),'.')
xlabel('latitudine')
ylabel('longitudine')

%GPS SPEED
%lat_metri =
%lon_metri = 
%v_lat = gradient(lat_metri, timebase);
%v_lon = gradient(lon_metri, timebase);
%GPSspeed = sqrt(v_lat^2 + v_lon^2);

%CALCOLO DELL'ENERGIA
t_start = 800;
t_end = timebase(end);
i_start = find(abs(timebase - t_start) < 1,  1);         %cerca l'indice di t_start; l'uguaglianza è considerata vera nell'intorno di un secondo (aumentare raggio dell'intorno se arriva meno di un pacchetto al secondo)
i_end = find(abs(timebase - t_end) < 1,  1);
W = Data(i_start:i_end, 2).*Data(i_start:i_end, 3);  % Watt = V * A
E = trapz(timebase(i_start:i_end), W)   /  3.6*1e-6;    %conversione Joule-kWh;  SCEGLIERE VALORE FINALE E INIZIALE DI INTEGRAZIONE

