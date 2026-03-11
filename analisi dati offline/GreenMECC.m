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
cycle_rate = 2; %the view is updated every cycle_rate seconds
%=========================================================== 
%here it starts the cycle that updates the view every "cycle_rate" seconds


[M, t_last] = mylaps(Data, t_last);


%M = equalize(M);

Lap = cell2mat(M(1));

LapData = uifigure(Name="Lap Data", NumberTitle="off");
g = uigridlayout(LapData); %Cuts the window into a grid
g.RowHeight = {'0.5x', 22, 22, 22, 22, 22, 22, 22, '3x'};
g.ColumnWidth = {100,'1x'};
ax = uiaxes(g); %We need this as the parents for the plot function
ax.Layout.Row = [1 9];
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
live = uicheckbox(g, "Text", "Live");
DataType = uidropdown(g, ...
    Items=["Voltage", "Current", "Speed", "Energy", "Power", "Distance", "Position"], ...
    ItemsData=[2 3 4 8 7 9 5]);
Compare = uidropdown(g, ...
    Items=["None", "Voltage", "Current", "Speed", "Energy", "Power", "Distance"], ...
    ItemsData=[0 2 3 4 8 7 9], ...
    ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, src, Lap2Number, cbx.Value));
DataType.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, src, Compare, Lap2Number, cbx.Value);
LapNumber.ValueChangedFcn=@(src,event) updatePlot(ax, M, src, DataType, Compare, Lap2Number, cbx.Value);
Lap2Number.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, src, cbx.Value);
cbx.ValueChangedFcn=@(src,event) updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, src.Value);
live.Value = 1;

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

while isgraphics(LapData)
    if live.Value
        Data = readmatrix(full_filename);
        [M, t_last] = mylaps(Data, t_last);
        updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value);
        %{
        try
            updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value);
        catch ME
            if strcmp(ME.identifier, "MATLAB:badsubscript")
                warning("Data is not ready yet")
                LapNumber.Value = LapNumber.Value - 1;
                if Lap2Number.Value ~= 0
                    Lap2Number.Value = Lap2Number.Value - 1;
                end
                updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value);
            end
        end
        %}
    end
    pause(cycle_rate);
end

%{
function loopUpdate(flag)
    while flag == true
        Data = readmatrix(full_filename);
        M = mylaps(Data);
        updatePlot(ax, M, LapNumber, DataType, Compare, Lap2Number, cbx.Value);
    end
end
%}

function updatePlot(ax, M, src, src2, src3, src4, flag)
    grid(ax, "on");
    lgd = legend(ax);
    xl = xlabel(ax, "Time (s)");
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
    
    if flag == 1 %Plot everything
        src3.Value = 0;
        src3.Enable = "off";
        src4.Value = 0;
        src4.Enable = "off";
        plotEverything(ax, M, D);
        return
    end

    data = cell2mat(M(val)); %NOT IN ANY IF STATEMENTS

    if isempty(data)
        fprintf("Data is not ready yet \n")
        for e=1:11
            if isempty(cell2mat(M(e)))
                src.Value = e-1;
                break
            end
        end
        
        return
    end

    if D == 5 %Position option
        plot(ax, data(:,5),data(:,6), 'LineWidth',2.0);
        xl.Visible = "off";
        return
    end
    if C2~=0 %Compare two laps
        src3.Value = 0;
        src3.Enable = "off";
        data2 = cell2mat(M(C2));
        if isempty(data2)
            fprintf("Data is not ready yet \n")
            src4.Value = 0;     
            return
        end
        plot(ax, data(:,1), data(:,D), data2(:,1), data2(:,D), 'LineWidth',2.0);
        xl.String = "Time (s)";
        switch D
        case 2
            yl.String = "Voltage (V)";
        case 3
            yl.String = "Current (mA)";
        case 4
            yl.String = "Speed (Km/h)";
        end
        return
    end
    if C~=0 %Compare data types within one lap
        plot(ax, data(:,1),data(:,D),data(:,1),data(:,C), 'LineWidth',2.0);
        yl.Visible = "off";
        switch D
        case 2
            legend1 = "Voltage (V)";
        case 3
            legend1 = "Current (mA)";
        case 4
            legend1 = "Speed (Km/h)";
        case 8
            legend1 = "Energy (mJ)";
        case 7
            legend1 = "Power (mW)";
        case 9
            legend1 = "Distance (Km)";
        end
        switch C
        case 2
            legend2 = "Voltage (V)";
        case 3
            legend2 = "Current (mA)";
        case 4
            legend2 = "Speed (Km/h)";
        case 8
            legend2 = "Energy (mJ)";
        case 7
            legend2 = "Power (mW)";
        case 9
            legend2 = "Distance (Km)";
        end
        lgd.Visible = "on";
        lgd.String = [legend1, legend2];
        return
    end
    plot(ax, data(:,1),data(:,D), 'LineWidth',2.0);
    xl.String = "Time (s)";
    switch D
    case 2
        yl.String = "Voltage (V)";
    case 3
        yl.String = "Current (mA)";
    case 4
        yl.String = "Speed (Km/h)";
    case 8
        yl.String = "Energy (mJ)";
        lgd.Visible = "on";
        lgd.String = "Total energy: " + data(end, D) ;
    case 7
        yl.String = "Power (mW)";
    case 9
        yl.String = "Distance (Km)";

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

function Me = equalize(M)
    Me = cell(1,11);
    for n = 1:11
        s = M(n, 1, 1);
        Me(n) = (M(n, :, 1)-s)/1e6;
    end
end

function[M, t_last] = mylaps(Data, t_last)
    M = cell(1,11);
    l = 1; % M index
    prev = 0;
    startl=0;
    endl=0;
    for t = 1:length( Data(:,1) )
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
            l = l+1;
            startl = 0; 
            endl = 0;
            %fprintf('lap %d completato\n',l);
        end  
        
        prev = curr;
    end
    t_last = length(Data(:,1));
end

%{
function energy = energy(time, power)
    
end
%}

