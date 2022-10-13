//
//  ContentView.swift
//  nfctest
//
//  Created by Stu Stakoff on 10/12/22.
//

import SwiftUI

struct ContentView: View {
    
    @EnvironmentObject var mr: MyReader
    
    var body: some View {
        VStack {
            Image(systemName: "globe")
                .imageScale(.large)
                .foregroundColor(.accentColor)
            if (mr.msglog.isEmpty) {
                Text("Waiting")
            } else {
                Text(mr.msglog.joined(separator: "\n"))
            }
            
            Button("Read") {
                mr.read()
            }
        }
        .padding()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
