Data = readmatrix('datif.csv');
M = mylaps(Data);

Lap = cell2mat(M(1));

LapData = uifigure(Name="Lap Data", NumberTitle="off");
g = uigridlayout(LapData);
g.RowHeight = {'0.5x', 22, 22, 22, '3x'};
g.ColumnWidth = {100,'1x'};
ax = uiaxes(g);
ax.Layout.Row = [2 5];
ax.Layout.Column = 2;
plot(ax, Lap(:,1), Lap(:,2));
LapNumber = uidropdown(g, ...
    Items=["Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[1 2 3 4 5 6 7 8 9 10 11]);
DataType = uidropdown(g, ...
    Items=["Voltage", "Current", "Speed", "Position", "Energy", "Power"], ...
    ItemsData=[2 3 4 5 7 8]);
Compare = uidropdown(g, ...
    Items=["None", "Voltage", "Current", "Speed", "Energy", "Power"], ...
    ItemsData=[0 2 3 4 7 8], ...
    ValueChangedFcn=@(src,event) updatePlot(LapNumber, M, DataType, src, ax));
DataType.ValueChangedFcn=@(src,event) updatePlot(LapNumber, M, src, Compare, ax);
LapNumber.ValueChangedFcn=@(src,event) updatePlot(src, M, DataType, Compare, ax);
%Poistioning of drop downs
LapNumber.Layout.Row = 2;
LapNumber.Layout.Column = 1;
DataType.Layout.Row = 3;
DataType.Layout.Column = 1;
Compare.Layout.Row = 4;
Compare.Layout.Column = 1;

function updatePlot(src, M, src2, src3, ax)
    val = src.Value;
    D = src2.Value;
    C = src3.Value;
    data = cell2mat(M(val));
    if D == 5
        plot(ax, data(:,5),data(:,6));
        return
    end
    if C~=0
        plot(ax, data(:,1),data(:,D),data(:,1),data(:,C));
        return
    end
    plot(ax, data(:,1),data(:,D));
end
