clear
clc

if exist("simulated_data.csv", "file")
    full_filename = "simulated_data.csv";
else
    [filename, pathname] = uigetfile( ...
        {'*.csv;', 'CSV file (*.csv)';
         '*.xlsx',  'Excel Spreadsheet file (*.xlsx)'; ...
         '*.*',  'All Files (*.*)'}, 'Pick a File to Import');
    full_filename = fullfile(pathname, filename);
end

Data = readmatrix(full_filename);

t_last = 1;  %the function mylaps requires the last index of time array; for the first cycle this is 1
M = cell(1,11);
l = 1;
cycle_rate = 2; %the view is updated every cycle_rate seconds
%=========================================================== 
%here it starts the cycle that updates the view every "cycle_rate" seconds


[M, t_last, l] = mylaps(Data, M, t_last, l);


Lap = cell2mat(M(1));

LapData = uifigure(Name="Lap Data", NumberTitle="off");
g = uigridlayout(LapData); %Cuts the window into a grid
g.RowHeight = {'0.5x', 22, 22, 22, 22, 22, 22, 22, 22, '3x'};
g.ColumnWidth = {100,'1x'};
ax = uiaxes(g); %We need this as the parents for the plot function
ax.Layout.Row = [1 10];
ax.Layout.Column = 2;
plot(ax, Lap(:,1), Lap(:,2), 'LineWidth',2.0);
grid(ax, "on");
xl = xlabel(ax, "Time (s)");
yl = ylabel(ax, "Voltage (V)");
LapNumber = uidropdown(g, ...
    Items=["Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[1 2 3 4 5 6 7 8 9 10 11]);
Lap2Number = uidropdown(g, ...
    Items=["None", "Lap1", "Lap2", "Lap3", "Lap4", "Lap5", "Lap6", "Lap7", "Lap8", "Lap9", "Lap10", "Lap11"], ...
    ItemsData=[0 1 2 3 4 5 6 7 8 9 10 11]);
lbl = uilabel(g, "Text", "Compare to:");
cbx = uicheckbox(g, "Text", "All");
DataType = uidropdown(g, ...
    Items=["Voltage", "Current", "Speed", "Energy", "Power", "Distance", "Position"], ...
    ItemsData=[2 3 4 8 7 9 5]);
Compare = uidropdown(g, ...
    Items=["None", "Voltage", "Current", "Speed", "Energy", "Power", "Distance"], ...
    ItemsData=[0 2 3 4 8 7 9]);
live = uicheckbox(g, "Text", "Live");
distance = uicheckbox(g, "Text", "vs Distance");

LapNumber.ValueChangedFcn=@(src,event) updatePlot(ax, M, src, DataType, Compare, Lap2Number, cbx.Value, distance.Value);
Lap2Number.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, src, cbx.Value, distance.Value);
cbx.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, src.Value, distance.Value);
DataType.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, src, Compare, Lap2Number, cbx.Value, distance.Value);
Compare.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, src, Lap2Number, cbx.Value, distance.Value);
distance.ValueChangedFcn=@(src, event) updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value, src.Value);
live.Value = 0;

%Poistioning of drop downs
LapNumber.Layout.Row = 2;
LapNumber.Layout.Column = 1;
DataType.Layout.Row = 3;
DataType.Layout.Column = 1;
Compare.Layout.Row = 4;
Compare.Layout.Column = 1;
lbl.Layout.Row = 5;
lbl.Layout.Column = 1;
Lap2Number.Layout.Row = 6;
Lap2Number.Layout.Column = 1;
cbx.Layout.Row = 7;
cbx.Layout.Column = 1;
live.Layout.Row = 8;
live.Layout.Column = 1;
distance.Layout.Row = 9;
distance.Layout.Column = 1;

currentLap = 1;
while isgraphics(LapData)
    LapNumber.ValueChangedFcn=@(src,event) lapChange(live, ax, M, src, DataType, Compare, Lap2Number, cbx.Value, distance.Value);
    Lap2Number.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, src, cbx.Value, distance.Value);
    cbx.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, src.Value, distance.Value);
    DataType.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, src, Compare, Lap2Number, cbx.Value, distance.Value);
    Compare.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, src, Lap2Number, cbx.Value, distance.Value);
    if live.Value
        Data = readmatrix(full_filename);
        [M, t_last, l] = mylaps(Data, M, t_last, l);
        for e=1:11
            if isempty(M{e})
                LapNumber.Value = e-1;
                break
            end
        end
%{        
        if ~isempty(M{currentLap+1})
            currentLap = currentLap + 1;
        end
%}
        %LapNumber.Value = currentLap;
        updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value, distance.Value);
    end
    pause(cycle_rate);
end


function lapChange(live, ax, M, src, DataType, Compare, Lap2Number, Value, vs)
    live.Value = 0;
    updatePlot(ax, M, src, DataType, Compare, Lap2Number, Value, vs)
end

function updatePlot(ax, M, src, src2, src3, src4, flag, vs)
    grid(ax, "on");
    lgd = legend(ax);
    xl = xlabel(ax, "");
    yl = ylabel(ax, "");
    xl.Visible = "on";
    yl.Visible = "on";
    lgd.Visible = "off";
    src3.Enable = "on";
    src4.Enable = "on";
    val = src.Value; %Get value of first dropdown: The laps
    D = src2.Value; %Get value of second dropdown: The data type
    C = src3.Value; %Get value of third dropdown: Compare with what datatype
    C2 = src4.Value; %Get value of fourth dropdown: Compare with another Lap
    switch vs
        case 0 
            X = 1;
            xl.String = "Time (s)";
        case 1
            X = 9;
            xl.String = "Distance (m)";
    end
    
    if flag == 1 %Plot everything
        src3.Value = 0;
        src3.Enable = "off";
        src4.Value = 0;
        src4.Enable = "off";
        plotEverything(ax, M, D);
        return
    end
    
    %Check if data ready and avoid error and a crash
    if isempty(M{val})
        fprintf("Data is not ready yet \n")
        
        for e=1:11
            if isempty(M{e})
                src.Value = e-1;
                break
            end
        end
        
        return
    end

    data = cell2mat(M(val)); %NOT IN ANY IF STATEMENTS


    %Needs fixing
    if D == 5 %Position option
        plot(ax, data(:,5),data(:,6), 'LineWidth',2.0);
        xl.Visible = "off";
        return
    end
    
    if C2~=0 %Compare two laps
        src3.Value = 0;
        src3.Enable = "off";
        
        if isempty(M{C2}) || C2==val
            fprintf("Data is not ready yet or comparing same lap \n")
            src4.Value = 0;     
            src3.Enable = "on";
            return
        end

        data2 = cell2mat(M(C2));
        plot(ax, data2(:,X), data2(:,D), data(:,X), data(:,D), 'LineWidth',2.0);
        lgd.Visible = "on";
        lgd.String = ["Lap: " + C2,"Lap: " + val];
        switch D
        case 2
            yl.String = "Voltage (V)";
        case 3
            yl.String = "Current (mA)";
        case 4
            yl.String = "Speed (m/s)";
        case 8
            yl.String = "Energy (mJ)";
            lgd.Visible = "on";
            lgd.String = ["Total energy of Lap " + C2 + ": " + data2(end, D),"Total energy of Lap " + val + ": " + data(end, D)] ;
        case 7
            yl.String = "Power (mW)";
        case 9
            yl.String = "Distance (m)";
        end
        return
    end
    if C~=0 %Compare data types within one lap
        plot(ax, data(:,X),data(:,D),data(:,X),data(:,C), 'LineWidth',2.0);
        yl.Visible = "off";
        switch D
        case 2
            legend1 = "Voltage (V)";
        case 3
            legend1 = "Current (mA)";
        case 4
            legend1 = "Speed (m/s)";
        case 8
            legend1 = "Energy (mJ)";
        case 7
            legend1 = "Power (mW)";
        case 9
            legend1 = "Distance (m)";
        end
        switch C
        case 2
            legend2 = "Voltage (V)";
        case 3
            legend2 = "Current (mA)";
        case 4
            legend2 = "Speed (m/s)";
        case 8
            legend2 = "Energy (mJ)";
        case 7
            legend2 = "Power (mW)";
        case 9
            legend2 = "Distance (m)";
        end
        lgd.Visible = "on";
        lgd.String = [legend1, legend2];
        return
    end
    plot(ax, data(:,X),data(:,D), 'LineWidth',2.0);
    switch D
    case 2
        yl.String = "Voltage (V)";
    case 3
        yl.String = "Current (mA)";
    case 4
        yl.String = "Speed (m/s)";
    case 8
        yl.String = "Energy (mJ)";
        lgd.Visible = "on";
        lgd.String = "Total energy: " + data(end, D) ;
    case 7
        yl.String = "Power (mW)";
    case 9
        yl.String = "Distance (m)";

    end
end

function plotEverything(ax, M, D)
    
    for e=1:11
        if isempty(cell2mat(M(e)))
            break
        end
        Lap = cell2mat(M(e));
        plot(ax, Lap(:,1), Lap(:,D));
        ax.NextPlot = "add";
    end
    ax.NextPlot = "replacechildren";
end

function[M, t_last, l] = mylaps(Data, M, t_last, l)
    prev = 0;
    startl=0;
    endl=0;
    for t = t_last:length( Data(:,1) )
        curr = Data(t,4);
    
        if prev==0 && curr > 0
            startl = t;    
        end
    
        if (prev > 0 && curr == 0) || (t == length(Data(:,1)) && not(curr==0) )
            endl = t;
            
        end
        
        if not(startl == 0) && not(endl == 0) %I modified the code here a bit to normalize the x axis
            temp = Data(startl: endl, :);
            smallTime = temp(1,1);
            temp(:,1) = (temp(:,1)-smallTime)/1e6;
            temp(:,7) = temp(:,2).*temp(:,3); %Adding power column
            temp(:,8) = cumtrapz(temp(:,1), temp(:,7)); %Energy
            temp(:,9) = cumtrapz(temp(:,1), temp(:,4)); %Distance
            M{l} = temp;
            if (t == length(Data(:,1)) && not(curr==0) )
                %do nothing
            else
                l = l+1;
            end 
            startl = 0; 
            endl = 0;
            %fprintf('lap %d completato\n',l);
        end  
        
        prev = curr;
    end
    t_last = length(Data(:,1));
end
