%codice di Luca Pagani
close all 
clc
clear 
% 1 tempo micro s 
% 2 tensione v
% 3 cbatt 
% 4 cmot 
% 5 speed m/s 
% 6 lat 
% 7 long 
time=linspace(60352400,271635000,1e4);
% time=[0:0.1:Data(end,1)];
Data=readmatrix("p1.csv");         %inserire nome file.csv da visualizzare
 V0=interp1(Data(:,1),Data(:,2),time);
 A0=interp1(Data(:,1),Data(:,3),time);
 P=V0.*A0;
 E=trapz(time.*1e-6,P);


% data = readmatrix('serial_datapp1.CSV');
% col5 = Data(:,5);
% 
% col5_clean = filloutliers(col5,'linear');
% Data(:,5)=col5_clean;
figure(1)
subplot(5,1,1)
plot(Data(:,1),Data(:,2),'.')
xlabel('time micro-s')
ylabel('tensione batteria[V]')
grid on 
% figure()
subplot(5,1,2)
plot(Data(:,1),Data(:,3),'.')
xlabel('time micro-s')
ylabel('corrente batteria [A]')
grid on 
% figure()
subplot(5,1,3)
plot(Data(:,1),(Data(:,4)./1023*5-2.5)/0.185,'.')
xlabel('time micro-s')
ylabel('corrente motore [A]')
% figure()
grid on 
subplot(5,1,4)
plot(Data(:,1),Data(:,5).*2*pi/60*0.28*3.6,'.')
xlabel('time micro-s')
ylabel('speed [km/h]')
 ylim([0,35]);
 grid on 
% figure()
subplot(5,1,5)
plot(Data(:,6),Data(:,7),'.')
xlabel('latitudine')
ylabel('longitudine')
% vbat ABatt speed - time 
% lat long

hold on
%================================================================

% 1 tempo micro s 
% 2 tensione v
% 3 cbatt 
% 4 cmot 
% 5 speed m/s 
% 6 lat 
% 7 long 
time=linspace(60352400,271635000,1e4);
% time=[0:0.1:Data(end,1)];
Data=readmatrix("simulazione1.csv");         %inserire nome file.csv da visualizzare
 V0=interp1(Data(:,1),Data(:,2),time);
 A0=interp1(Data(:,1),Data(:,3),time);
 P=V0.*A0;
 E=trapz(time.*1e-6,P);


% data = readmatrix('serial_datapp1.CSV');
% col5 = Data(:,5);
% 
% col5_clean = filloutliers(col5,'linear');
% Data(:,5)=col5_clean;
figure(2)
subplot(5,1,1)
plot(Data(:,1),Data(:,2),'.')
xlabel('time micro-s')
ylabel('tensione batteria[V]')
grid on 
% figure()
subplot(5,1,2)
plot(Data(:,1),Data(:,3),'.')
xlabel('time micro-s')
ylabel('corrente batteria [A]')
grid on 
% figure()
subplot(5,1,3)
plot(Data(:,1),(Data(:,4)./1023*5-2.5)/0.185,'.')
xlabel('time micro-s')
ylabel('corrente motore [A]')
% figure()
grid on 
subplot(5,1,4)
plot(Data(:,1),Data(:,5).*2*pi/60*0.28*3.6,'.')
xlabel('time micro-s')
ylabel('speed [km/h]')
 ylim([0,35]);
 grid on 
% figure()
subplot(5,1,5)
plot(Data(:,6),Data(:,7),'.')
xlabel('latitudine')
ylabel('longitudine')
% vbat ABatt speed - time 
% lat long

