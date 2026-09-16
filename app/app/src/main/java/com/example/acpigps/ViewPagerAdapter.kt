package com.example.acpigps

import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter

class ViewPagerAdapter(fragmentActivity: FragmentActivity) : FragmentStateAdapter(fragmentActivity) {

    val compassFragment = CompassFragment()
    val bleDebugFragment = BLEDebugFragment()
    private val fragments = listOf(compassFragment, bleDebugFragment)

    override fun getItemCount(): Int = fragments.size

    override fun createFragment(position: Int): Fragment = fragments[position]
}
