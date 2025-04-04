import SwiftUI

struct FeederView: View {
    @State private var feeder: Feeder
    @State private var isLoading = false
    @State private var errorMessage: String? = nil
    @State private var updateMessage: String? = nil
    @State private var allFoodEvents: [FoodEvent] = []
    @State private var todayFoodEvents: [FoodEvent] = []
    @State private var allGateEvents: [GateEvent] = []
    @State private var todayGateEvents: [GateEvent] = []
    @State private var showDetails = false
    @State private var totalDispensed: Double = 0.0
    @State private var showResetFoodQuantity: Bool = false
    @State private var resetFoodQuantity: Double = 0.0
    @State private var selectedDate = Date()
    @State private var isDatePickerVisible = true
    
    var updateFeeder: (Feeder, Bool) -> Void
    
    init(feeder: Feeder, updateFeeder: @escaping (Feeder, Bool) -> Void)
    {
        _feeder = State(initialValue: feeder)
        self.updateFeeder = updateFeeder
    }
    
    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                if isLoading {
                    ProgressView("Loading...")
                        .progressViewStyle(CircularProgressViewStyle())
                        .padding()
                } else {
                    VStack {
                        Text(feeder.name)
                            .font(.largeTitle)
                            .fontWeight(.bold)
                            .foregroundColor(.primary)
                        Text(feeder.id)
                            .font(.footnote)
                            .foregroundColor(.secondary)
                            .padding(.bottom, 10)
                        
                        
                        HStack
                        {
                            // Weight Circle
                            Circle()
                                .fill(Color.green.opacity(0.2))
                                .frame(width: 200, height: 200)
                                .overlay(
                                    VStack
                                    {
                                        Text("Weight")
                                            .font(.caption)
                                            .foregroundColor(.gray)
                                            .padding(.bottom, 3)
                                        
                                        Text("\(feeder.foodCurrentWeight, specifier: "%.1f") gr")
                                            .font(.title2)
                                            .foregroundColor(.green)
                                        
                                        Text("\(timeDifference(from: feeder.lastFoodCurrentWeightUpdateTime))")
                                            .font(.caption)
                                            .foregroundColor(.gray)
                                            .padding(.top, 3)
                                    }
                                )
                                .padding(.horizontal, 10)
                            
                            let currentRemainingFood = getRemainingFood()
                            let foodStorageQuantity = feeder.foodStorageQuantity // Example full storage capacity
                            let remainingPercentage = currentRemainingFood / foodStorageQuantity // Value between 0.0 and 1.0
                            
                            // Remaining Food Progress Circle
                            ZStack
                            {
                                // Background Circle
                                Circle()
                                    .stroke(Color.gray.opacity(0.2), lineWidth: 20)
                                    .frame(width: 200, height: 200)
                                
                                // Foreground Circle (Dynamic Gradient)
                                Circle()
                                    .trim(from: 0, to: remainingPercentage) // Dynamic arc based on remaining food
                                    .stroke(
                                        LinearGradient(
                                            gradient: Gradient(colors: [Color.green, Color.orange, Color.red]),
                                            startPoint: .top,
                                            endPoint: .bottom
                                        ),
                                        style: StrokeStyle(lineWidth: 20, lineCap: .round)
                                    )
                                    .rotationEffect(.degrees(-90)) // Start progress from top
                                    .frame(width: 200, height: 200)
                                    .animation(.easeInOut, value: remainingPercentage)
                                
                                // Text Overlay
                                VStack
                                {
                                    Text("Remaining Food")
                                        .font(.caption)
                                        .foregroundColor(.gray)
                                    
                                    Text("\(currentRemainingFood, specifier: "%.1f") kg")
                                        .font(.title)
                                        .fontWeight(.bold)
                                        .foregroundColor(Color.green)
                                        .padding(.top, 2)
                                    
                                    Text("Refilled \(timeDifference(from: feeder.lastFoodStorageQuantityUpdateTime))")
                                        .font(.caption2)
                                        .foregroundColor(.gray)
                                        .padding(.top, 4)
                                    
                                    Button("Refill")
                                    {
                                        showResetFoodQuantity.toggle()
                                    }
                                    .font(.subheadline)
                                    .padding(.horizontal, 10)
                                    .padding(.vertical, 10)
                                    .foregroundColor(.white)
                                    .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                                    .cornerRadius(15)
                                    .shadow(color: Color.black.opacity(0.2), radius: 5, x: 0, y: 5)
                                    .sheet(isPresented: $showResetFoodQuantity) {
                                        RefillFoodPopupView(foodStorageQuantity: feeder.foodStorageQuantity, onSave: { updatedQuantity, updatedTime in
                                            feeder.foodStorageQuantity = updatedQuantity
                                            feeder.lastFoodStorageQuantityUpdateTime = updatedTime
                                            
                                            updateFeeder(feeder, true) // Also updates it it bd
                                        })
                                    }
                                }
                            }
                            .padding(.horizontal, 10)
                        }
                        
                        
                        // Error or Update Messages
                        if let errorMessage = errorMessage {
                            Text(errorMessage)
                                .foregroundColor(.red)
                        }
                        
                        if let updateMessage = updateMessage {
                            Text(updateMessage)
                                .foregroundColor(.green)
                                .padding(.top, 10)
                        }
                    }
                    
                    Spacer()
                    
                    // FeedConfigView
                    VStack(alignment: .leading, spacing: 10) {
                        Text("Feed Configuration")
                            .font(.headline)
                            .foregroundColor(.primary)
                            .frame(maxWidth: .infinity)
                            .multilineTextAlignment(.center)
                            .bold()
                        
                        FeedConfigView(feedConfig: $feeder.feedConfig, feederID: $feeder.id, feederPassword: $feeder.password) { updatedConfig in
                            feeder.feedConfig = updatedConfig
                            updateFeeder(feeder, true) // Update feeder in parent
                        }
                    }
                    .padding()
                    .background(
                        RoundedRectangle(cornerRadius: 15)
                            .fill(Color.blue.opacity(0.1))
                            .shadow(color: Color.gray.opacity(0.3), radius: 5, x: 0, y: 2)
                    )
                    .padding(.top)
                    
                    // Display circles for Gate Events and Food Dispensed
                    VStack
                    {
                        HStack
                        {
                            Spacer()
                            DatePicker("", selection: $selectedDate, in: ...Date(), displayedComponents: .date)
                                .datePickerStyle(.compact)
                                .frame(width: 150)
                                .onChange(of: selectedDate) { newValue in
                                    
                                    self.todayGateEvents = self.allGateEvents.filter { event in
                                        Calendar.current.isDate(newValue, inSameDayAs: Date(timeIntervalSince1970: TimeInterval(event.startTime)))
                                    }
                                    
                                    self.todayFoodEvents = self.allFoodEvents.filter { event in
                                        Calendar.current.isDate(newValue, inSameDayAs: Date(timeIntervalSince1970: TimeInterval(event.dispensedAt)))
                                    }
                                    
                                    let total = self.todayFoodEvents.reduce(0.0) { $0 + $1.quantityDispensed }
                                    self.totalDispensed = total
                                }
                            Spacer()
                        }
                        .padding()
                        
                        HStack(spacing: 20)
                        {
                            // Gate Events Circle
                            VStack
                            {
                                Text("Gate Events")
                                    .font(.caption)
                                    .foregroundColor(.gray)
                                
                                Circle()
                                    .fill(Color.purple.opacity(0.2))
                                    .frame(width: 150, height: 150)
                                    .overlay(
                                        VStack
                                        {
                                            Text("Visits Count")
                                                .font(.caption)
                                                .foregroundColor(.gray)
                                            Text("\(todayGateEvents.count)")
                                                .font(.title2)
                                                .foregroundColor(.purple)
                                        }
                                    )
                                    .onTapGesture
                                {
                                    withAnimation
                                    {
                                        showDetails.toggle()
                                    }
                                }
                                
                                if showDetails
                                {
                                    GeometryReader
                                    { geometry in
                                        ScrollView(.horizontal, showsIndicators: true)
                                        {
                                            HStack
                                            {
                                                if todayGateEvents.count < 4
                                                {
                                                    Spacer(minLength: (geometry.size.width - CGFloat(todayGateEvents.count * 100)) / 2)
                                                }
                                                
                                                ForEach(todayGateEvents)
                                                { event in
                                                    GateEventCircle(event: event)
                                                        .frame(width: 100)
                                                }
                                                
                                                if todayGateEvents.count < 4
                                                {
                                                    Spacer(minLength: (geometry.size.width - CGFloat(todayGateEvents.count * 100)) / 2)
                                                }
                                            }
                                            .frame(height: 100)
                                        }
                                    }
                                    .frame(height: 120)
                                    .padding(.top, 10)
                                }
                            }
                            
                            // Food Dispensed Circle
                            VStack {
                                Text("Food Dispensed")
                                    .font(.caption)
                                    .foregroundColor(.gray)
                                
                                Circle()
                                    .fill(Color.green.opacity(0.2))
                                    .frame(width: 150, height: 150)
                                    .overlay(
                                        VStack {
                                            Text("Dispensed Total")
                                                .font(.caption)
                                                .foregroundColor(.gray)
                                            Text("\(totalDispensed * 1000, specifier: "%.1f") g")
                                                .font(.title2)
                                                .foregroundColor(.green)
                                        }
                                    )
                                    .onTapGesture {
                                        withAnimation {
                                            showDetails.toggle()
                                        }
                                    }
                                
                                if showDetails {
                                    GeometryReader { geometry in
                                        ScrollView(.horizontal, showsIndicators: true) {
                                            HStack {
                                                if todayFoodEvents.count < 4 {
                                                    Spacer(minLength: (geometry.size.width - CGFloat(todayFoodEvents.count * 100)) / 2)
                                                }
                                                
                                                ForEach(todayFoodEvents, id: \.id) { event in
                                                    FoodEventCircle(event: event)
                                                        .frame(width: 100)
                                                }
                                                
                                                if todayFoodEvents.count < 4 {
                                                    Spacer(minLength: (geometry.size.width - CGFloat(todayFoodEvents.count * 100)) / 2)
                                                }
                                            }
                                            .frame(height: 100)
                                        }
                                    }
                                    .frame(height: 120)
                                    .padding(.top, 10)
                                }
                            }
                        }
                    }
                }
            }
            .padding()
        }
        .onAppear {
            refreshFeederFromDB()
            fetchFoodDispenseEvents()
            fetchGateEvents()
        }
        .refreshable {
            isLoading = true
            refreshFeederFromDB()
            fetchFoodDispenseEvents()
            fetchGateEvents()
        }
    }
    
    private func refreshFeederFromDB()
    {
        isLoading = true
        
        guard let url = URL(string: "https://dev.bull-software.com/get_feeder.php?ID=\(feeder.id)&Password=\(feeder.password)") else {
            errorMessage = "Invalid URL"
            SetIsLoadingFalseAfterOneSec()
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, response, error in
            DispatchQueue.main.async {
                
                if let error = error {
                    errorMessage = "Error: \(error.localizedDescription)"
                    updateMessage = nil
                    SetIsLoadingFalseAfterOneSec()
                    return
                }
                
                guard let data = data else {
                    errorMessage = "No data received"
                    updateMessage = nil
                    SetIsLoadingFalseAfterOneSec()
                    return
                }
                
                do {
                    print(data);
                    
                    let updatedFeederData = try JSONDecoder().decode(FeederResponse.self, from: data)
                    self.feeder = Feeder(
                        id: updatedFeederData.ID,
                        password: feeder.password,
                        name: updatedFeederData.Name,
                        trapMode: updatedFeederData.TrapMode,
                        feedConfig: updatedFeederData.FeedFoodConfiguration,
                        foodStorageQuantity: updatedFeederData.FoodStorageQuantity,
                        lastFoodStorageQuantityUpdateTime: updatedFeederData.LastFoodStorageQuantityUpdateTime,
                        foodCurrentWeight: updatedFeederData.FoodCurrentWeight,
                        lastFoodCurrentWeightUpdateTime: updatedFeederData.LastFoodCurrentWeightUpdateTime
                    )
                    
                    print(self.feeder);
                    
                    // cache the feeder data into memory
                    updateFeeder(feeder, false)
                    
                    updateMessage = "Feeder data updated successfully."
                    SetIsLoadingFalseAfterOneSec()
                } catch {
                    errorMessage = "Failed to decode data"
                    updateMessage = nil
                    SetIsLoadingFalseAfterOneSec()
                }
            }
            
            SetIsLoadingFalseAfterOneSec()
        }.resume()
    }
    
    private func SetIsLoadingFalseAfterOneSec()
    {
        DispatchQueue.main.asyncAfter(deadline: .now() + 1)
        {
            self.isLoading = false
        }
    }
    
    // Fetch Food Dispense Events
    private func fetchFoodDispenseEvents()
    {
        guard let url = URL(string: "https://dev.bull-software.com/get_food_dispense_events.php?ID=\(feeder.id)&Password=\(feeder.password)") else {
            errorMessage = "Invalid URL for events"
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, _, error in
            DispatchQueue.main.async {
                if let error = error {
                    errorMessage = "Error: \(error.localizedDescription)"
                    return
                }
                
                guard let data = data else {
                    errorMessage = "No data received"
                    return
                }
                
                do {
                    let allEvents = try JSONDecoder().decode([FoodEvent].self, from: data)
                    let todayEvents = allEvents.filter { event in
                        Calendar.current.isDateInToday(Date(timeIntervalSince1970: TimeInterval(event.dispensedAt)))
                    }
                    self.todayFoodEvents = todayEvents
                    
                    let total = todayEvents.reduce(0.0) { $0 + $1.quantityDispensed }
                    self.totalDispensed = total
                    
                    self.allFoodEvents = allEvents
                } catch {
                    errorMessage = "Failed to decode events"
                }
            }
        }.resume()
    }
    
    // Fetch Gate Events
    private func fetchGateEvents() {
        guard let url = URL(string: "https://dev.bull-software.com/get_gate_events.php?ID=\(feeder.id)&Password=\(feeder.password)") else {
            errorMessage = "Invalid URL for gate events"
            return
        }
        
        URLSession.shared.dataTask(with: url) { data, _, error in
            DispatchQueue.main.async {
                if let error = error {
                    errorMessage = "Error: \(error.localizedDescription)"
                    return
                }
                
                guard let data = data else {
                    errorMessage = "No data received"
                    return
                }
                
                do {
                    let allEvents = try JSONDecoder().decode([GateEvent].self, from: data)
                    let todayEvents = allEvents.filter { event in
                        Calendar.current.isDateInToday(Date(timeIntervalSince1970: TimeInterval(event.startTime)))
                    }
                    
                    self.allGateEvents = allEvents
                    self.todayGateEvents = todayEvents
                } catch {
                    errorMessage = "Failed to decode gate events"
                }
            }
        }.resume()
    }
    
    func timeDifference(from unixTime: Int) -> String
    {
        let currentTime = Date().timeIntervalSince1970
        let difference = Int(currentTime) - unixTime
        
        if difference < 60 {
            return "\(difference) second\(difference == 1 ? "" : "s") ago"
        } else if difference < 3600 {
            let minutes = difference / 60
            return "\(minutes) minute\(minutes == 1 ? "" : "s") ago"
        } else if difference < 86400 {
            let hours = difference / 3600
            return "\(hours) hour\(hours == 1 ? "" : "s") ago"
        } else {
            let days = difference / 86400
            return "\(days) day\(days == 1 ? "" : "s") ago"
        }
    }
    
    func getRemainingFood() -> Double {
        let storageQuantity = feeder.foodStorageQuantity
        let storageQuantityUpdatedTime = feeder.lastFoodStorageQuantityUpdateTime
        
        // Filter events that occurred after the storage quantity was last updated
        let relevantEvents = allFoodEvents.filter { $0.dispensedAt > storageQuantityUpdatedTime }
        
        // Calculate the total quantity dispensed in the relevant events
        let totalDispensed = relevantEvents.reduce(0.0) { $0 + $1.quantityDispensed }
        
        // Subtract the total dispensed quantity from the storage quantity
        let remainingFood = storageQuantity - totalDispensed
        
        // Ensure the result is not negative
        return max(remainingFood, 0.0)
    }
}


struct FoodEvent: Codable, Identifiable {
    let id: Int
    let feederID: String
    let dispensedAt: Int
    let quantityDispensed: Double
    
    enum CodingKeys: String, CodingKey
    {
        case id = "ID"
        case feederID = "feederID"
        case dispensedAt = "dispensedAt"
        case quantityDispensed = "quantityDispensed"
    }
}

struct GateEvent: Codable, Identifiable {
    let id: Int
    let startTime: Int
    let endTime: Int
    
    enum CodingKeys: String, CodingKey {
        case id = "ID"
        case startTime = "startTime"
        case endTime = "endTime"
    }
    
    var duration: Int {
        return endTime - startTime
    }
}

struct FoodEventCircle: View {
    let event: FoodEvent
    
    var body: some View {
        VStack {
            Circle()
                .fill(Color.blue.opacity(0.2))
                .frame(width: 60, height: 60)
                .overlay(
                    Text("\(event.quantityDispensed * 1000, specifier: "%.1f") g")
                        .font(.footnote)
                        .foregroundColor(.blue)
                )
            
            Text(formatTime(from: event.dispensedAt))
                .font(.caption)
                .foregroundColor(.gray)
        }
    }
    
    func formatTime(from timestamp: Int) -> String {
        let date = Date(timeIntervalSince1970: TimeInterval(timestamp))
        let formatter = DateFormatter()
        formatter.timeStyle = .short
        return formatter.string(from: date)
    }
}

struct GateEventCircle: View {
    let event: GateEvent
    
    var body: some View {
        VStack {
            Circle()
                .fill(Color.purple.opacity(0.2))
                .frame(width: 60, height: 60)
                .overlay(
                    Text("\(event.duration) s")
                        .font(.footnote)
                        .foregroundColor(.purple)
                )
            
            Text(formatTime(from: event.startTime))
                .font(.caption)
                .foregroundColor(.gray)
        }
    }
    
    func formatTime(from timestamp: Int) -> String {
        let date = Date(timeIntervalSince1970: TimeInterval(timestamp))
        let formatter = DateFormatter()
        formatter.timeStyle = .short
        return formatter.string(from: date)
    }
}

struct FeedItem: Identifiable {
    let id: String  // Use time as a unique identifier
    let time: String
    var quantity: Int
}

struct FeedConfigView: View {
    @Binding var feedConfig: [String: Int]
    @Binding var feederID: String
    @Binding var feederPassword: String
    var onConfigUpdate: ( [String: Int] ) -> Void
    @State private var selectedItem: FeedItem?
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 10) {
                ForEach(feedConfig.sorted(by: { $0.key < $1.key }).map { FeedItem(id: $0.key, time: $0.key, quantity: $0.value) }) { item in
                    RoundedRectangle(cornerRadius: 15)
                        .fill(Color.blue.opacity(0.2))
                        .frame(width: 120, height: 60)
                        .overlay(
                            VStack {
                                Text(item.time)
                                    .font(.headline)
                                Text("\(item.quantity) gr")
                                    .font(.subheadline)
                            }
                                .foregroundColor(.black)
                        )
                        .onTapGesture {
                            selectedItem = item
                        }
                }
            }
            .padding()
        }
        .sheet(item: $selectedItem) { item in
            EditFoodConfigPopupView(item: item, onSave: { updatedTime, updatedQuantity in
                if updatedTime != item.time {
                    feedConfig.removeValue(forKey: item.time)
                }
                feedConfig[updatedTime] = updatedQuantity
                onConfigUpdate(feedConfig) // Notify the parent
            }, onRemove: { itemId in
                feedConfig.removeValue(forKey: itemId)
                onConfigUpdate(feedConfig) // Notify the parent
            })
        }
        .cornerRadius(30)
        .shadow(radius: 30)
        
        HStack
        {
            Button(action: {
                let newTime = generateUniqueTime()
                feedConfig[newTime] = 30 // Default quantity
                onConfigUpdate(feedConfig)
            }){
                Text("Add New")
                    .font(.headline)
                    .padding(.horizontal, 20)
                    .padding(.vertical, 10)
                    .foregroundColor(.white)
                    .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                    .cornerRadius(15)
                    .shadow(color: Color.black.opacity(0.2), radius: 5, x: 0, y: 5)
                
            }
            .frame(maxWidth: .infinity)
            .multilineTextAlignment(.center)
            
            Button(action:{
                setCommandToEsp32(command: "UpdateFeeder", feederId: feederID, feederPassword: feederPassword);
            })
            {
                Text("Update feeder")
                    .font(.headline)
                    .padding(.horizontal, 20)
                    .padding(.vertical, 10)
                    .foregroundColor(.white)
                    .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                    .cornerRadius(15)
                    .shadow(color: Color.black.opacity(0.2), radius: 5, x: 0, y: 5)
                
            }
            .frame(maxWidth: .infinity)
            .multilineTextAlignment(.center)
            
            Button(action:{
                setCommandToEsp32(command: "DispenseNow_10", feederId: feederID, feederPassword: feederPassword);
            })
            {
                Text("Dispense Now")
                    .font(.headline)
                    .padding(.horizontal, 20)
                    .padding(.vertical, 10)
                    .foregroundColor(.white)
                    .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                    .cornerRadius(15)
                    .shadow(color: Color.black.opacity(0.2), radius: 5, x: 0, y: 5)
                
            }
            .frame(maxWidth: .infinity)
            .multilineTextAlignment(.center)
        }
    }
    
    private func generateUniqueTime() -> String {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm"
        
        let now = Date()
        let calendar = Calendar.current
        let minute = calendar.component(.minute, from: now)
        let minuteRounded = (minute / 30) * 30
        let roundedDate = calendar.date(
            bySettingHour: calendar.component(.hour, from: now),
            minute: minuteRounded,
            second: 0,
            of: now
        ) ?? now
        
        var time = formatter.string(from: roundedDate)
        var counter = 0
        
        while feedConfig.keys.contains(time) {
            counter += 1
            if let newDate = calendar.date(byAdding: .minute, value: counter * 2, to: roundedDate) {
                time = formatter.string(from: newDate)
            }
        }
        
        return time
    }
    
    private func setCommandToEsp32(command: String, feederId: String, feederPassword: String)
    {
        // Define the URL for the PHP API that inserts commands
        guard let url = URL(string: "https://dev.bull-software.com/add_esp32_command.php?ID=\(feederId)&Password=\(feederPassword)&Command=\(command)") else
        {
            print("Invalid URL")
            return
        }
        
        // Create the request
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        
        // Create the JSON body with the required parameters
        let requestBody: [String: Any] = [
            "ID": feederId,
            "Password": feederPassword,
            "Command": command
        ]
        
        do {
            // Convert the request body dictionary to JSON data
            let jsonData = try JSONSerialization.data(withJSONObject: requestBody, options: [])
            
            // Set the HTTP body of the request
            request.httpBody = jsonData
        } catch {
            print("Error serializing JSON: \(error)")
            return
        }
        
        // Create a data task to send the request
        let task = URLSession.shared.dataTask(with: request) { data, response, error in
            // Handle any errors
            if let error = error {
                print("Error sending command: \(error.localizedDescription)")
                return
            }
            
            // Check if the response is valid
            if let response = response as? HTTPURLResponse {
                if response.statusCode == 200 {
                    // Success
                    print("Command sent successfully!")
                    if let data = data {
                        // Parse the response (optional)
                        do {
                            let responseObject = try JSONSerialization.jsonObject(with: data, options: [])
                            print("Response: \(responseObject)")
                        } catch {
                            print("Error parsing response: \(error)")
                        }
                    }
                } else {
                    // Handle non-200 response
                    print("Failed to send command. Status code: \(response.statusCode)")
                }
            }
        }
        
        // Start the data task
        task.resume()
    }
    
}


struct EditFoodConfigPopupView: View {
    @Environment(\.presentationMode) var presentationMode
    let item: FeedItem
    var onSave: (String, Int) -> Void
    var onRemove: (String) -> Void  // Closure to handle remove action
    
    @State private var selectedHour: Int
    @State private var selectedMinute: Int
    @State private var quantity: Int
    
    init(item: FeedItem, onSave: @escaping (String, Int) -> Void, onRemove: @escaping (String) -> Void) {
        self.item = item
        self.onSave = onSave
        self.onRemove = onRemove
        
        let timeComponents = item.time.split(separator: ":").map { Int($0) ?? 0 }
        _selectedHour = State(initialValue: timeComponents[0])
        _selectedMinute = State(initialValue: timeComponents[1])
        _quantity = State(initialValue: item.quantity)
    }
    
    var body: some View {
        VStack(spacing: 50) {
            // Time Pickers
            HStack(spacing: 10) {
                Text("Dispense Time: ")
                    .foregroundColor(.blue)
                    .font(.title)
                    .padding(.horizontal, 10)
                
                Picker("Hour", selection: $selectedHour) {
                    ForEach(0..<24) { hour in
                        Text(String(format: "%02d", hour)).tag(hour)
                    }
                }
                .pickerStyle(WheelPickerStyle())
                .frame(width: 80, height: 100)
                .clipped()
                
                Text(":")
                    .font(.title)
                    .foregroundColor(.gray)
                
                Picker("Minute", selection: $selectedMinute) {
                    ForEach(0..<60) { minute in
                        if minute % 1 == 0 {
                            Text(String(format: "%02d", minute)).tag(minute)
                        }
                    }
                }
                .pickerStyle(WheelPickerStyle())
                .frame(width: 80, height: 100)
                .clipped()
            }
            
            // Quantity Stepper with custom style
            HStack {
                Text("Quantity:")
                    .foregroundColor(.blue)
                    .font(.title)
                    .padding(.horizontal, 10)
                
                Stepper(value: $quantity, in: 5...30, step: 1) {
                    Text("\(quantity) gr")
                        .font(.title)
                        .bold()
                        .padding(.horizontal, 10)
                        .frame(maxWidth: 300)
                        .background(Color.blue.opacity(0.1))
                        .cornerRadius(10)
                }
                .padding(.horizontal)
            }
            
            // Action Buttons with styling
            HStack {
                Button("Cancel") {
                    presentationMode.wrappedValue.dismiss()
                }
                .foregroundColor(.white)
                .padding()
                .background(Color.red)
                .cornerRadius(10)
                .shadow(radius: 5)
                
                Spacer()
                
                Button("Save") {
                    let updatedTime = String(format: "%02d:%02d", selectedHour, selectedMinute)
                    onSave(updatedTime, quantity)
                    presentationMode.wrappedValue.dismiss()
                }
                .foregroundColor(.white)
                .padding()
                .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                .cornerRadius(10)
                .shadow(radius: 5)
                
                Spacer()
                
                // Remove Button
                Button("Remove") {
                    onRemove(item.id) // Pass the ID to remove the item
                    presentationMode.wrappedValue.dismiss()
                }
                .foregroundColor(.white)
                .padding()
                .background(Color.red.opacity(0.7))
                .cornerRadius(10)
                .shadow(radius: 5)
            }
            .padding(.horizontal)
            .padding(.vertical, 30)
        }
    }
}


struct RefillFoodPopupView: View {
    @Environment(\.presentationMode) var presentationMode
    let foodStorageQuantity: Double
    var onSave: (Double, Int) -> Void
    
    @State private var quantity: Double
    
    init(foodStorageQuantity: Double, onSave: @escaping (Double, Int) -> Void) {
        self.foodStorageQuantity = foodStorageQuantity
        self.onSave = onSave
        _quantity = State(initialValue: foodStorageQuantity)
    }
    
    var body: some View {
        VStack(spacing: 20)
        {
            // Quantity Stepper with custom style
            HStack {
                Text("Quantity:")
                    .foregroundColor(.blue)
                    .font(.title)
                    .padding(.horizontal, 10)
                
                Stepper(value: $quantity, in: 0.5...5, step: 0.5) {
                    Text(String(format: "%.1f kg", quantity))
                        .font(.title)
                        .bold()
                        .padding(.horizontal, 10)
                        .frame(maxWidth: 300)
                        .background(Color.blue.opacity(0.1))
                        .cornerRadius(10)
                }
                .padding(.horizontal)
            }
            
            // Action Buttons with styling
            HStack {
                Button("Cancel") {
                    presentationMode.wrappedValue.dismiss()
                }
                .foregroundColor(.white)
                .padding()
                .background(Color.red)
                .cornerRadius(10)
                .shadow(radius: 5)
                
                Spacer()
                
                Button("Save") {
                    let currentUnixTime = Int(Date().timeIntervalSince1970)
                    onSave(quantity, currentUnixTime)
                    presentationMode.wrappedValue.dismiss()
                }
                .foregroundColor(.white)
                .padding()
                .background(LinearGradient(gradient: Gradient(colors: [Color.mint, Color.teal]), startPoint: .leading, endPoint: .trailing))
                .cornerRadius(10)
                .shadow(radius: 5)
            }
            .padding(.horizontal)
            .padding(.vertical, 30)
        }
    }
}
