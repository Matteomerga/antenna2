Data = readmatrix('datif.csv');
M = mylaps(Data);

Lap = cell2mat(M(1));

LapData = figure(Name="Lap Data", NumberTitle="off");
plot(Lap(:,1), Lap(:,2));
LapNumber = uidropdown(LapData, ...
    Items=["Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[1 2 3 4 5 6 7 8 9 10 11], ...
    Position=[10 500 100 22]);
DataType = uidropdown(LapData, ...
    Items=["Voltage", "Current", "Speed", "Position"], ...
    ItemsData=[2 3 4 5], ...
    Position=[10 470 100 22]);
Compare = uidropdown(LapData, ...
    Items=["None", "Voltage", "Current", "Speed"], ...
    ItemsData=[0 2 3 4], ...
    ValueChangedFcn=@(src,event) updatePlot(LapNumber, M, DataType, src), ...
    Position=[10 440 100 22]);
DataType.ValueChangedFcn=@(src,event) updatePlot(LapNumber, M, src, Compare);
LapNumber.ValueChangedFcn=@(src,event) updatePlot(src, M, DataType, Compare);

function updatePlot(src, M, src2, src3)
    val = src.Value;
    D = src2.Value;
    C = src3.Value;
    data = cell2mat(M(val));
    if D == 5
        plot(data(:,5),data(:,6));
        return
    end
    if C~=0
        plot(data(:,1),data(:,D),data(:,1),data(:,C));
        return
    end
    plot(data(:,1),data(:,D));
end